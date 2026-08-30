#include "ChunkMeshGeneration.h"

#include <array>
#include <cfloat>
#include <cstring>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Logger.h"
#include "engine/scene/Axis.h"
#include "foundation/util/BitOperations.h"
#include "game/blocks/BlockTypes.h"

typedef unsigned int** BinaryPlaneArray;
typedef unsigned int* BinaryPlane;

struct CompactChunkFace {
    CompactChunkVertex vertices[4];
    unsigned int indices[6];
};

static void zeroFillPlanes(BinaryPlaneArray planes, size_t size) {
    for (size_t slice = 0; slice < size; slice++) {
        std::memset(planes[slice], 0, size * sizeof(unsigned int));
    }
}

static BinaryPlaneArray allocateBinaryPlanes(size_t size) {
    BinaryPlaneArray planes = new BinaryPlane[size];
    for (size_t slice = 0; slice < size; slice++) {
        planes[slice] = new unsigned int[size]{};
    }
    return planes;
}

static void freeBinaryPlanes(BinaryPlaneArray planes, size_t size) {
    for (size_t slice = 0; slice < size; slice++) {
        delete[] planes[slice];
    }
    delete[] planes;
}

static glm::ivec3 axisToCoord(Axis axis, int slice, int row, int column) {
    // Utility for greedy meshing.
    // Look onto the axis from the positive, then pick the positiv axis as height (rowindexed) wich has the other
    // positive axis imediatly 90° clockwise.
    switch (axis) {
        case Axis::X: return glm::ivec3(slice, column, row);
        case Axis::Y: return glm::ivec3(row, slice, column);
        case Axis::Z: return glm::ivec3(column, row, slice);
        default: return glm::ivec3(0);  // Unhandled case
    }
}

static BoundingBox calculateChunkMeshBounds(const std::vector<CompactChunkVertex>& vertexBuffer) {
    BoundingBox bounds = BoundingBox::invalid();
    for (const CompactChunkVertex& vertex : vertexBuffer) {
        bounds.min = glm::min(bounds.min, glm::vec3(vertex.getPosition()));
        bounds.max = glm::max(bounds.max, glm::vec3(vertex.getPosition()));
    }
    return bounds;
}

static CompactChunkFace generateCompactChunkFace(
    const glm::ivec3& origin,
    AxisDirection faceDirection,
    FaceInfo fInfo,
    int width = 1,
    int height = 1
) {
    // Vertices for a specific face
    glm::ivec3 v0;
    glm::ivec3 v1;
    glm::ivec3 v2;
    glm::ivec3 v3;

    // Generate the appropriate face based on FaceDirection
    switch (faceDirection) {
        case AxisDirection::PositiveX:                   // RIGHT
            v0 = origin + glm::ivec3(1, width, height);  // Front Top Right
            v1 = origin + glm::ivec3(1, width, 0);       // Back Top Right
            v2 = origin + glm::ivec3(1, 0, 0);           // Back Bottom Right
            v3 = origin + glm::ivec3(1, 0, height);      // Front Bottom Right
            break;
        case AxisDirection::NegativeX:                   // LEFT
            v0 = origin + glm::ivec3(0, width, 0);       // Back Top Left
            v1 = origin + glm::ivec3(0, width, height);  // Front Top Left
            v2 = origin + glm::ivec3(0, 0, height);      // Front Bottom Left
            v3 = origin + glm::ivec3(0, 0, 0);           // Back Bottom Left
            break;
        case AxisDirection::PositiveY:                   // TOP
            v0 = origin + glm::ivec3(0, 1, 0);           // Back Top Left
            v1 = origin + glm::ivec3(height, 1, 0);      // Back Top Right
            v2 = origin + glm::ivec3(height, 1, width);  // Front Top Right
            v3 = origin + glm::ivec3(0, 1, width);       // Front Top Left
            break;
        case AxisDirection::NegativeY:                   // BOTTOM
            v0 = origin + glm::ivec3(0, 0, width);       // Front Bottom Left
            v1 = origin + glm::ivec3(height, 0, width);  // Front Bottom Right
            v2 = origin + glm::ivec3(height, 0, 0);      // Back Bottom Right
            v3 = origin + glm::ivec3(0, 0, 0);           // Back Bottom Left
            break;
        case AxisDirection::PositiveZ:                   // FRONT
            v0 = origin + glm::ivec3(0, height, 1);      // Top Left
            v1 = origin + glm::ivec3(width, height, 1);  // Top Right
            v2 = origin + glm::ivec3(width, 0, 1);       // Bottom Right
            v3 = origin + glm::ivec3(0, 0, 1);           // Bottom Left
            break;
        case AxisDirection::NegativeZ:                   // BACK
            v0 = origin + glm::ivec3(width, height, 0);  // Top Right
            v1 = origin + glm::ivec3(0, height, 0);      // Top Left
            v2 = origin + glm::ivec3(0, 0, 0);           // Bottom Left
            v3 = origin + glm::ivec3(width, 0, 0);       // Bottom Right
            break;
    }

    // Quickfix for wrong uvs
    if (faceDirection == AxisDirection::PositiveZ || faceDirection == AxisDirection::NegativeZ) {
        std::swap(height, width);
    }

    // UV coordinates scaled by width and height (63 is max value since UVCoord stores 6 bit values)
    UVCoord uv00 = {0, 0};
    UVCoord uv10 = {static_cast<uint8_t>(height), 0};
    UVCoord uv01 = {0, static_cast<uint8_t>(width)};
    UVCoord uv11 = {static_cast<uint8_t>(height), static_cast<uint8_t>(width)};

    CompactChunkFace face = {
        {CompactChunkVertex(v0, 0, 0, fInfo.texIndex, uv00, faceDirection),
         CompactChunkVertex(v1, 0, 0, fInfo.texIndex, uv10, faceDirection),
         CompactChunkVertex(v2, 0, 0, fInfo.texIndex, uv11, faceDirection),
         CompactChunkVertex(v3, 0, 0, fInfo.texIndex, uv01, faceDirection)},
        {
            0, 1, 2, 2, 3, 0  // indices
        }
    };

    return face;
}

CPURenderData<CompactChunkVertex> generateMeshForChunk(const Block* blocks, const BlockToTextureMap& texMap) {
    std::vector<CompactChunkVertex> vertexBuffer;
    std::vector<unsigned int> indexBuffer;

    unsigned int currentIndexOffset = 0;

    glm::ivec3 origin;
    for (origin.x = 0; origin.x < CHUNK_WIDTH; origin.x++) {
        for (origin.y = 0; origin.y < CHUNK_HEIGHT; origin.y++) {
            for (origin.z = 0; origin.z < CHUNK_DEPTH; origin.z++) {
                const Block& blockRef = blocks[chunkBlockIndex(origin.x, origin.y, origin.z)];
                if (blockRef.isSolid) {
                    // Check each face and add the appropriate face to the buffer if visible
                    for (AxisDirection dir : allAxisDirections) {
                        if (isBlockFaceVisible(blocks, origin.x, origin.y, origin.z, dir)) {
                            CompactChunkFace face = generateCompactChunkFace(
                                origin, dir, texMap.getInfo(blockRef.type, dir)
                            );
                            // Add face vertices to the global vertex buffer
                            for (int i = 0; i < 4; i++) {
                                vertexBuffer.push_back(face.vertices[i]);
                            }

                            // Add face indices to the global index buffer
                            for (int i = 0; i < 6; i++) {
                                indexBuffer.push_back(face.indices[i] + currentIndexOffset);
                            }
                            currentIndexOffset += 4;  // Update the index offset (each face has 4 vertices)
                        }
                    }
                }
            }
        }
    }

    BoundingBox bounds = calculateChunkMeshBounds(vertexBuffer);
    return {"Chunk", std::move(vertexBuffer), std::move(indexBuffer), bounds};
}

CPURenderData<CompactChunkVertex> generateMeshForChunkGreedy(const Block* blocks, const BlockToTextureMap& texMap) {
    // Hold for each blocktype cullplanes for all 3 axes
    std::unordered_map<uint16_t, BinaryPlaneArray[3]> blockTypeCullPlanes;

    // Populate culling planes with mesh data
    for (int x = 0; x < CHUNK_WIDTH; x++) {
        for (int y = 0; y < CHUNK_HEIGHT; y++) {
            for (int z = 0; z < CHUNK_DEPTH; z++) {
                const Block& blockRef = blocks[chunkBlockIndex(x, y, z)];
                if (blockRef.isSolid) {
                    BinaryPlaneArray* planes = nullptr;

                    // Check if axis planes for blocktype exist
                    auto it = blockTypeCullPlanes.find(blockRef.type);
                    if (it == blockTypeCullPlanes.end()) {
                        planes = blockTypeCullPlanes[blockRef.type];  // operator[] creates new value at key location

                        // Allocate Planes
                        for (Axis axis : {Axis::X, Axis::Y, Axis::Z}) {
                            planes[axis] = allocateBinaryPlanes(CHUNK_SIZE);
                        }
                    } else {
                        planes = it->second;
                    }

                    // Set value in all axis
                    planes[Axis::X][z][y] |= 1U << x;  // X-Cullplane
                    planes[Axis::Y][x][z] |= 1U << y;  // Y-Cullplane
                    planes[Axis::Z][y][x] |= 1U << z;  // Z-Cullplane
                }
            }
        }
    }

    std::vector<CompactChunkVertex> vertexBuffer;
    std::vector<unsigned int> indexBuffer;

    unsigned int currentIndexOffset = 0;

    // Two greedy meshing planes because forward and backwards direction can be face culled in a single iteration
    BinaryPlaneArray forwardGreedyMeshingPlanes = allocateBinaryPlanes(CHUNK_SIZE);
    BinaryPlaneArray backwardGreedyMeshingPlanes = allocateBinaryPlanes(CHUNK_SIZE);

    for (auto& element : blockTypeCullPlanes) {
        for (Axis axis : allAxis) {
            BinaryPlaneArray cullPlanes = element.second[axis];

            AxisDirection forward;
            AxisDirection backward;
            if (axis == Axis::X) {
                // X-Axis
                forward = AxisDirection::PositiveX;
                backward = AxisDirection::NegativeX;
            } else if (axis == Axis::Y) {
                // Y-Axis
                forward = AxisDirection::PositiveY;
                backward = AxisDirection::NegativeY;
            } else {
                // Z-Axis
                forward = AxisDirection::PositiveZ;
                backward = AxisDirection::NegativeZ;
            }

            // Reset for eaxh axis (Reuse of memory)
            zeroFillPlanes(forwardGreedyMeshingPlanes, CHUNK_SIZE);
            zeroFillPlanes(backwardGreedyMeshingPlanes, CHUNK_SIZE);

            // Face culling
            for (int slice = 0; slice < CHUNK_SIZE; slice++) {
                for (int row = 0; row < CHUNK_SIZE; row++) {
                    // Cull forward and backwards faces
                    unsigned int culledForwardMask = cullPlanes[slice][row] & ~(cullPlanes[slice][row] >> 1U);
                    unsigned int culledBackwardMask = cullPlanes[slice][row] & ~(cullPlanes[slice][row] << 1U);

                    // Insert culled values into greedy meshing planes
                    while (culledForwardMask != 0) {
                        unsigned int column = trailing_zeros(culledForwardMask);
                        culledForwardMask &= culledForwardMask - 1;                // Clear least significant bit
                        forwardGreedyMeshingPlanes[column][slice] |= (1U << row);  // Culling operation
                    }

                    while (culledBackwardMask != 0) {
                        unsigned int column = trailing_zeros(culledBackwardMask);
                        culledBackwardMask &= culledBackwardMask - 1;               // Clear least significant bit
                        backwardGreedyMeshingPlanes[column][slice] |= (1U << row);  // Culling operation
                    }
                }
            }

            // Greedy meshing in forward and backward direction
            for (int dir = 0; dir < 2; dir++) {
                AxisDirection currentDirection;
                BinaryPlaneArray greedyMeshingPlanes;
                if (dir == 0) {  // Forward
                    currentDirection = forward;
                    greedyMeshingPlanes = forwardGreedyMeshingPlanes;
                } else {  //  Backward
                    currentDirection = backward;
                    greedyMeshingPlanes = backwardGreedyMeshingPlanes;
                }

                for (int slice = 0; slice < CHUNK_SIZE; slice++) {
                    for (int row = 0; row < CHUNK_SIZE; row++) {
                        int column = 0;
                        while (column < CHUNK_SIZE) {
                            column += trailing_zeros(greedyMeshingPlanes[slice][row] >> column);

                            if (column >= CHUNK_SIZE) break;  // Row processed

                            unsigned int w = trailing_ones(greedyMeshingPlanes[slice][row] >> column);  // Width in row

                            if (w <= 0) break;  // No more blocks to process

                            unsigned int mask = createMask(w) << column;

                            unsigned int h = 1;
                            while (row + h < CHUNK_SIZE) {
                                if ((greedyMeshingPlanes[slice][row + h] & mask) != mask) {
                                    break;  // Can no longer expand in height
                                }
                                greedyMeshingPlanes[slice][row + h] &= ~mask;  // Nuke bits that have been expanded too
                                h++;
                            }

                            glm::ivec3 coord = axisToCoord(axis, slice, row, column);

                            // ##### Adding face #####
                            CompactChunkFace face = generateCompactChunkFace(
                                coord, currentDirection, texMap.getInfo(element.first, currentDirection), w, h
                            );
                            // Add face vertices to the global vertex buffer
                            for (int i = 0; i < 4; i++) {
                                vertexBuffer.push_back(face.vertices[i]);
                            }

                            // Add face indices to the global index buffer
                            for (int i = 0; i < 6; i++) {
                                indexBuffer.push_back(face.indices[i] + currentIndexOffset);
                            }
                            currentIndexOffset += 4;  // Update the index offset (each face has 4 vertices)
                            // ##### End Adding face #####

                            column += w;
                        }
                    }
                }
            }

            // Free allocated plane of this blocktype and axis since thats no longer needed now
            freeBinaryPlanes(element.second[axis], CHUNK_SIZE);
        }
    }

    freeBinaryPlanes(forwardGreedyMeshingPlanes, CHUNK_SIZE);
    freeBinaryPlanes(backwardGreedyMeshingPlanes, CHUNK_SIZE);

    BoundingBox bounds = calculateChunkMeshBounds(vertexBuffer);
    return {"Chunk", std::move(vertexBuffer), std::move(indexBuffer), bounds};
}