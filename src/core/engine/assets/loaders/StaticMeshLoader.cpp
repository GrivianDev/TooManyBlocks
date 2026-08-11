#include "StaticMeshLoader.h"

#include <GL/glew.h>

#include <cstring>
#include <fstream>
#include <unordered_map>

#include "Logger.h"

#define OBJ_PREFIX_V_POS       "v"
#define OBJ_PREFIX_V_TEX_COORD "vt"
#define OBJ_PREFIX_V_NORMAL    "vn"
#define OBJ_PREFIX_FACE        "f"
#define OBJ_IDENT_OBJECT       "o"
#define OBJ_IDENT_GROUP        "g"

namespace std {  // Needed for the hash map
    template <>
    struct hash<Vertex> {
        size_t operator()(const Vertex& vertex) const {
            size_t hash1 = hash<float>()(vertex.position.x) ^ (hash<float>()(vertex.position.y) << 1) ^
                           (hash<float>()(vertex.position.z) << 2);
            size_t hash2 = hash<float>()(vertex.uv.x) ^ (hash<float>()(vertex.uv.y) << 1);
            size_t hash3 = hash<float>()(vertex.normal.x) ^ (hash<float>()(vertex.normal.y) << 1) ^
                           (hash<float>()(vertex.normal.z) << 2);
            return hash1 ^ hash2 ^ hash3;
        }
    };
}  // namespace std

static BoundingBox calculateMeshBounds(const std::vector<Vertex>& vertexBuffer) {
    BoundingBox bounds = BoundingBox::invalid();
    for (const Vertex& vertex : vertexBuffer) {
        bounds.min = glm::min(bounds.min, vertex.position);
        bounds.max = glm::max(bounds.max, vertex.position);
    }
    return bounds;
}

CPURenderData<Vertex> loadStaticMeshFromObjFile(const std::string& objFilePath, bool flipWinding) {
    std::vector<CPURenderData<Vertex>> meshes;
    {
        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> uvs;
        std::vector<glm::vec3> normals;

        CPURenderData<Vertex> currentMesh;
        currentMesh.name = "Mesh";  // Default name in case none is provided

        std::unordered_map<Vertex, unsigned int> vertexMap;  // Map to indices to reuse vertices

        std::ifstream file(objFilePath);
        if (!file.is_open()) {
            throw std::runtime_error("Error: Could not open file " + objFilePath);
        }

        std::string line;
        char prefix[4];
        while (std::getline(file, line)) {
            std::istringstream ss(line);
            ss >> prefix;

            if (std::strcmp(prefix, OBJ_PREFIX_V_POS) == 0) {
                glm::vec3 pos;
                ss >> pos.x >> pos.y >> pos.z;
                positions.push_back(pos);
            } else if (std::strcmp(prefix, OBJ_PREFIX_V_TEX_COORD) == 0) {
                glm::vec2 tex;
                ss >> tex.x >> tex.y;
                uvs.push_back(tex);
            } else if (std::strcmp(prefix, OBJ_PREFIX_V_NORMAL) == 0) {
                glm::vec3 normal;
                ss >> normal.x >> normal.y >> normal.z;
                normals.push_back(normal);
            } else if (std::strcmp(prefix, OBJ_PREFIX_FACE) == 0) {
                std::vector<unsigned int> faceIndices;
                std::string vertexInfo;

                // Parse each vertex (istringstream parses till whitespace character)
                while (ss >> vertexInfo) {
                    std::istringstream vs(vertexInfo);
                    std::string indexStr;
                    unsigned int vIdx = 0, vtIdx = 0, vnIdx = 0;

                    // Parse indices v/vt/vn (Vertex / UV /Normal)
                    // 1 is substracted cause indices in .obj start at 1
                    std::getline(vs, indexStr, '/');
                    vIdx = std::stoi(indexStr) - 1;
                    // UV is optional
                    std::getline(vs, indexStr, '/');
                    vtIdx = !indexStr.empty() ? std::stoi(indexStr) - 1 : 0;
                    // Normal is also optional
                    std::getline(vs, indexStr);  // Parse rest
                    vnIdx = !indexStr.empty() ? std::stoi(indexStr) - 1 : 0;

                    // Create vertex (if uv or normal are missing defaulting to 0)
                    Vertex vertex = {
                        positions[vIdx],
                        uvs.size() > vtIdx ? uvs[vtIdx] : glm::vec2(0.0f),
                        normals.size() > vnIdx ? normals[vnIdx] : glm::vec3(0.0f)
                    };

                    // Check if vertex already exists in the map
                    if (vertexMap.find(vertex) == vertexMap.end()) {
                        // If not, insert new index and push new vertex
                        vertexMap[vertex] = static_cast<unsigned int>(currentMesh.vertices.size());
                        currentMesh.vertices.push_back(vertex);
                    }

                    // Add the index of the vertex
                    faceIndices.push_back(vertexMap[vertex]);
                }

                // Triangulate the face (convert to triangles)
                if (faceIndices.size() >= 3) {
                    for (size_t i = 1; i < faceIndices.size() - 1; i++) {
                        currentMesh.indices.push_back(faceIndices[0]);  // Base vertex for triangulation
                        if (flipWinding) {
                            currentMesh.indices.push_back(faceIndices[i + 1]);
                            currentMesh.indices.push_back(faceIndices[i]);
                        } else {
                            currentMesh.indices.push_back(faceIndices[i]);
                            currentMesh.indices.push_back(faceIndices[i + 1]);
                        }
                    }
                } else {
                    lgr::lout.warn("Ignoring a face with less than 3 vertices");
                }
            } else if (std::strcmp(prefix, OBJ_IDENT_GROUP) == 0 || std::strcmp(prefix, OBJ_IDENT_GROUP) == 0) {
                // Parse new object or group
                if (!currentMesh.vertices.empty()) {
                    meshes.push_back(std::move(currentMesh));
                    currentMesh = CPURenderData<Vertex>();
                    vertexMap.clear();
                }
                ss >> currentMesh.name;
            }
        }

        // Push the last mesh
        if (!currentMesh.vertices.empty()) {
            meshes.push_back(std::move(currentMesh));
        }
    }

    if (meshes.empty()) {
        throw std::runtime_error("Could not read any mesh data from obj file: " + objFilePath);
    }
    if (meshes.size() > 1) {
        lgr::lout.warn("There are multiple objects in that file, ignoring all except first object in: " + objFilePath);
    }

    CPURenderData<Vertex> meshData = std::move(meshes[0]);
    meshData.bounds = calculateMeshBounds(meshData.vertices);
    return meshData;
}

std::shared_ptr<RenderData> packToRenderData(const CPURenderData<Vertex>& data) {
    VertexBuffer vbo = VertexBuffer::create(data.vertices.data(), data.vertices.size() * sizeof(Vertex));

    VertexBufferLayout layout;
    layout.push(GL_FLOAT, 3);  // Position
    layout.push(GL_FLOAT, 2);  // UV
    layout.push(GL_FLOAT, 3);  // Normal
    vbo.setLayout(layout);

    VertexArray vao = VertexArray::create();
    vao.addBuffer(vbo);

    if (data.isIndexed()) {
        IndexBuffer ibo = IndexBuffer::create(data.indices.data(), data.indices.size());
        return std::make_shared<IndexedRenderData>(std::move(vao), std::move(vbo), std::move(ibo));
    } else {
        return std::make_shared<NonIndexedRenderData>(std::move(vao), std::move(vbo));
    }
}
