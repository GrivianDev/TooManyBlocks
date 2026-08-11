#include "SkeletalMeshLoader.h"

#include <GL/glew.h>
#include <json/JsonParser.h>
#include <stdint.h>

#include <cstring>
#include <fstream>

#include "Logger.h"

#define GLB_JSON_CHUNK   0x4E4F534A
#define GLB_BIN_CHUNK    0x004E4942
#define GLB_MAGIC_NUMBER 0x46546C67

CPUSkeletalMeshData loadSkeletalMeshFromGlbFile(const std::string& glbFilePath, bool flipWinding) {
    std::vector<Json::JsonValue> jsonChunks;
    std::vector<std::vector<char>> binaryChunks;

    {
        std::ifstream file(glbFilePath, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open file: " + glbFilePath);
        }

        uint32_t magicNumber = 0;
        file.read(reinterpret_cast<char*>(&magicNumber), sizeof(magicNumber));
        if (file.gcount() != sizeof(magicNumber) || magicNumber != GLB_MAGIC_NUMBER) {
            throw std::runtime_error("Error reading magic number from glb file " + glbFilePath);
        }

        uint32_t version = 0;
        file.read(reinterpret_cast<char*>(&version), sizeof(version));
        if (file.gcount() != sizeof(version)) {
            throw std::runtime_error("Error reading version from glb file " + glbFilePath);
        }

        uint32_t length = 0;
        file.read(reinterpret_cast<char*>(&length), sizeof(length));
        if (file.gcount() != sizeof(length)) {
            throw std::runtime_error("Error reading the total length of file " + glbFilePath);
        }

        uint32_t bytesRead = 12;  // already read magic, version, length
        // Read all chunk data in glb file. There are json and binary chunk type
        while (bytesRead < length) {
            uint32_t chunkLength, chunkType;
            file.read(reinterpret_cast<char*>(&chunkLength), sizeof(uint32_t));
            file.read(reinterpret_cast<char*>(&chunkType), sizeof(uint32_t));
            bytesRead += 2 * sizeof(uint32_t);

            std::vector<char> chunkData;
            chunkData.reserve(chunkLength);
            file.read(chunkData.data(), chunkLength);
            bytesRead += chunkLength;

            if (chunkType == GLB_JSON_CHUNK) {
                // Convert to string and parse it to json structure
                std::string json(chunkData.data(), chunkLength);
                jsonChunks.push_back(std::move(Json::parseJson(json)));
            } else if (chunkType == GLB_BIN_CHUNK) {
                binaryChunks.push_back(std::move(chunkData));
            } else {
                throw std::runtime_error("Encountered unknown glb chunk identifier");
            }
        }
    }

    // Parse data in application blueprints
    CPUSkeletalMeshData skData;
    skData.animatedMeshNodeIndex = -1;
    for (const Json::JsonValue& jsonChunkObj : jsonChunks) {
        try {
            const Json::JsonValue& activeScene = jsonChunkObj["scenes"][jsonChunkObj["scene"].toInt()];
            std::vector<Node> nodesArray;
            const Json::JsonArray& nodeArray = jsonChunkObj["nodes"].toArray();
            for (int nodeIndex = 0; nodeIndex < nodeArray.size(); nodeIndex++) {
                const Json::JsonObject& nodeObj = nodeArray[nodeIndex].toObject();
                Node node;

                // Get name
                auto it = nodeObj.find("name");
                if (it != nodeObj.end()) {
                    node.name = it->second.toString();
                }

                // Default to "no parent"
                node.parentIndex = -1;

                // Get optional translation
                it = nodeObj.find("translation");
                if (it != nodeObj.end()) {
                    glm::vec3 pos(0.0f);
                    const Json::JsonArray& translation = it->second.toArray();
                    for (int i = 0; i < translation.size(); i++) {
                        if (translation[i].isInt()) {
                            pos[i] = static_cast<float>(translation[i].toInt());
                        } else if (translation[i].isDouble()) {
                            pos[i] = static_cast<float>(translation[i].toDouble());
                        }
                    }
                    node.localTransform.setPosition(pos);
                }

                // Get optional rotation
                it = nodeObj.find("rotation");
                if (it != nodeObj.end()) {
                    glm::quat rot;
                    const Json::JsonArray& rotation = it->second.toArray();
                    for (int i = 0; i < rotation.size(); i++) {
                        if (rotation[i].isInt()) {
                            rot[i] = static_cast<float>(rotation[i].toInt());
                        } else if (rotation[i].isDouble()) {
                            rot[i] = static_cast<float>(rotation[i].toDouble());
                        }
                    }
                    node.localTransform.setRotation(rot);
                }

                // Get optional scale
                it = nodeObj.find("scale");
                if (it != nodeObj.end()) {
                    float factor = 0.0f;
                    const Json::JsonArray& scale = it->second.toArray();
                    for (int i = 0; i < scale.size(); i++) {
                        if (scale[i].isInt()) {
                            factor += static_cast<float>(scale[i].toInt());
                        } else if (scale[i].isDouble()) {
                            factor += static_cast<float>(scale[i].toDouble());
                        }
                    }
                    factor /= scale.size();  // Normalize to uniform scale
                    node.localTransform.setScale(factor);
                }

                // Parse child indices (each node can reference child nodes, parent indices are resolved later)
                it = nodeObj.find("children");
                if (it != nodeObj.end()) {
                    const Json::JsonArray& children = it->second.toArray();
                    node.childIndices.reserve(children.size());
                    for (const Json::JsonValue& childIndex : children) {
                        node.childIndices.push_back(childIndex.toInt());
                    }
                }

                // Test if its the mesh node that is animated
                it = nodeObj.find("mesh");
                if (it != nodeObj.end()) {
                    int meshIndex = it->second.toInt();
                    it = nodeObj.find("skin");
                    if (it != nodeObj.end()) {
                        // This is the animated mesh node
                        if (skData.animatedMeshNodeIndex != -1) {
                            throw std::runtime_error(
                                "More then one valid skinned mesh encountered in file: " + glbFilePath
                            );
                        }
                        int skinIndex = it->second.toInt();
                        skData.animatedMeshNodeIndex = nodeIndex;

                        // ########################### Parsing mesh stuff ################################
                        const Json::JsonValue& mesh = jsonChunkObj["meshes"][meshIndex];  // Index into meshes array
                        if (mesh["primitives"].toArray().size() > 1) {
                            lgr::lout.warn("Multiple values in primitive array for mesh, defaulting to first entry...");
                        }

                        const Json::JsonValue& primitivesEntry = mesh["primitives"][0];
                        const Json::JsonValue& attributes = primitivesEntry["attributes"];

                        // Parse position
                        const Json::JsonValue& positionAccessor = jsonChunkObj["accessors"]
                                                                              [attributes["POSITION"].toInt()];
                        unsigned int compType = static_cast<unsigned int>(positionAccessor["componentType"].toInt());
                        if (compType != GL_FLOAT) {
                            throw std::runtime_error("POSITION is not of component type float");
                        } else if (positionAccessor["type"].toString() != "VEC3") {
                            throw std::runtime_error("POSITION is not of type VEC3");
                        }

                        int vertexCount = positionAccessor["count"].toInt();
                        skData.meshData.vertices.resize(vertexCount);

                        {
                            const Json::JsonValue& bufferView = jsonChunkObj["bufferViews"]
                                                                            [positionAccessor["bufferView"].toInt()];
                            const std::vector<char>& binData = binaryChunks[bufferView["buffer"].toInt()];

                            const glm::vec3* src = reinterpret_cast<const glm::vec3*>(
                                binData.data() + bufferView["byteOffset"].toInt()
                            );
                            for (size_t i = 0; i < vertexCount; i++) {
                                skData.meshData.vertices[i].position = src[i];
                            }
                        }

                        // Parse UV
                        const Json::JsonValue& uvAccessor = jsonChunkObj["accessors"][attributes["TEXCOORD_0"].toInt()];
                        compType = static_cast<unsigned int>(uvAccessor["componentType"].toInt());
                        if (compType != GL_FLOAT) {
                            throw std::runtime_error("TEXCOORD_0 is not of component type float");
                        } else if (uvAccessor["type"].toString() != "VEC2") {
                            throw std::runtime_error("TEXCOORD_0 is not of type VEC2");
                        } else if (uvAccessor["count"].toInt() != vertexCount) {
                            throw std::runtime_error("Vertex attrib count mismatch");
                        }

                        {
                            const Json::JsonValue& bufferView = jsonChunkObj["bufferViews"]
                                                                            [uvAccessor["bufferView"].toInt()];
                            const std::vector<char>& binData = binaryChunks[bufferView["buffer"].toInt()];

                            const glm::vec2* src = reinterpret_cast<const glm::vec2*>(
                                binData.data() + bufferView["byteOffset"].toInt()
                            );
                            for (size_t i = 0; i < vertexCount; i++) {
                                skData.meshData.vertices[i].uv = src[i];
                            }
                        }

                        // Parse Normal
                        const Json::JsonValue& normalAccessor = jsonChunkObj["accessors"][attributes["NORMAL"].toInt()];
                        compType = static_cast<unsigned int>(normalAccessor["componentType"].toInt());
                        if (compType != GL_FLOAT) {
                            throw std::runtime_error("NORMAL is not of component type float");
                        } else if (normalAccessor["type"].toString() != "VEC3") {
                            throw std::runtime_error("NORMAL is not of type VEC3");
                        } else if (normalAccessor["count"].toInt() != vertexCount) {
                            throw std::runtime_error("Vertex attrib count mismatch");
                        }

                        {
                            const Json::JsonValue& bufferView = jsonChunkObj["bufferViews"]
                                                                            [normalAccessor["bufferView"].toInt()];
                            const std::vector<char>& binData = binaryChunks[bufferView["buffer"].toInt()];

                            const glm::vec3* src = reinterpret_cast<const glm::vec3*>(
                                binData.data() + bufferView["byteOffset"].toInt()
                            );
                            for (size_t i = 0; i < vertexCount; i++) {
                                skData.meshData.vertices[i].normal = src[i];
                            }
                        }

                        // Parse joint indices attrib (Indices my be stored as types GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT
                        // or GL_UNSIGNED_INT)
                        const Json::JsonValue& jointIdxAccessor = jsonChunkObj["accessors"]
                                                                              [attributes["JOINTS_0"].toInt()];
                        compType = static_cast<unsigned int>(jointIdxAccessor["componentType"].toInt());
                        if (compType != GL_UNSIGNED_BYTE && compType != GL_UNSIGNED_SHORT &&
                            compType != GL_UNSIGNED_INT) {
                            throw std::runtime_error("JOINTS_0 is not of compatible type");
                        } else if (jointIdxAccessor["type"].toString() != "VEC4") {
                            throw std::runtime_error("JOINTS_0 is not of type VEC4");
                        } else if (jointIdxAccessor["count"].toInt() != vertexCount) {
                            throw std::runtime_error("Vertex attrib count mismatch");
                        }

                        {
                            const Json::JsonValue& bufferView = jsonChunkObj["bufferViews"]
                                                                            [jointIdxAccessor["bufferView"].toInt()];
                            const std::vector<char>& binData = binaryChunks[bufferView["buffer"].toInt()];

                            // Broadcast to larger type
                            if (compType == GL_UNSIGNED_BYTE) {
                                const glm::u8vec4* src = reinterpret_cast<const glm::u8vec4*>(
                                    binData.data() + bufferView["byteOffset"].toInt()
                                );
                                for (size_t i = 0; i < vertexCount; i++) {
                                    skData.meshData.vertices[i].joints = glm::uvec4(src[i]);
                                }
                            } else if (compType == GL_UNSIGNED_SHORT) {
                                const glm::u16vec4* src = reinterpret_cast<const glm::u16vec4*>(
                                    binData.data() + bufferView["byteOffset"].toInt()
                                );
                                for (size_t i = 0; i < vertexCount; i++) {
                                    skData.meshData.vertices[i].joints = glm::uvec4(src[i]);
                                }
                            } else if (compType == GL_UNSIGNED_INT) {
                                const glm::uvec4* src = reinterpret_cast<const glm::uvec4*>(
                                    binData.data() + bufferView["byteOffset"].toInt()
                                );
                                for (size_t i = 0; i < vertexCount; i++) {
                                    skData.meshData.vertices[i].joints = src[i];  // already uvec4
                                }
                            } else {
                                throw std::runtime_error("Unsupported component type for joints attribute");
                            }
                        }

                        // Parse weights
                        const Json::JsonValue& weightAccessor = jsonChunkObj["accessors"]
                                                                            [attributes["WEIGHTS_0"].toInt()];
                        compType = static_cast<unsigned int>(weightAccessor["componentType"].toInt());
                        if (compType != GL_FLOAT) {
                            throw std::runtime_error("WEIGHTS_0 is not of component type float");
                        } else if (weightAccessor["type"].toString() != "VEC4") {
                            throw std::runtime_error("WEIGHTS_0 is not of type VEC4");
                        } else if (weightAccessor["count"].toInt() != vertexCount) {
                            throw std::runtime_error("Vertex attrib count mismatch");
                        }

                        {
                            const Json::JsonValue& bufferView = jsonChunkObj["bufferViews"]
                                                                            [weightAccessor["bufferView"].toInt()];
                            const std::vector<char>& binData = binaryChunks[bufferView["buffer"].toInt()];

                            const glm::vec4* src = reinterpret_cast<const glm::vec4*>(
                                binData.data() + bufferView["byteOffset"].toInt()
                            );
                            for (size_t i = 0; i < vertexCount; i++) {
                                skData.meshData.vertices[i].weights = src[i];
                            }
                        }

                        // Optional: Parse indices
                        auto it = primitivesEntry.toObject().find("indices");
                        if (it != primitivesEntry.toObject().end()) {
                            const Json::JsonValue& indexAccessor = jsonChunkObj["accessors"][it->second.toInt()];
                            const Json::JsonValue& bufferView = jsonChunkObj["bufferViews"]
                                                                            [indexAccessor["bufferView"].toInt()];

                            compType = static_cast<unsigned int>(indexAccessor["componentType"].toInt());
                            if (indexAccessor["type"].toString() != "SCALAR") {
                                throw std::runtime_error("Indexbuffer accessor type is not SCALAR");
                            } else if (compType != GL_UNSIGNED_BYTE && compType != GL_UNSIGNED_SHORT &&
                                       compType != GL_UNSIGNED_INT) {
                                throw std::runtime_error("Indexbuffer accessor component type is not supported");
                            }

                            const std::vector<char>& binData = binaryChunks[bufferView["buffer"].toInt()];
                            int indexCount = indexAccessor["count"].toInt();
                            skData.meshData.indices.reserve(indexCount);

                            // Broadcast to larger type
                            if (compType == GL_UNSIGNED_BYTE) {
                                const uint8_t* src = reinterpret_cast<const uint8_t*>(
                                    binData.data() + bufferView["byteOffset"].toInt()
                                );
                                for (size_t i = 0; i < indexCount; i++) {
                                    skData.meshData.indices.push_back(static_cast<uint32_t>(src[i]));
                                }
                            } else if (compType == GL_UNSIGNED_SHORT) {
                                const uint16_t* src = reinterpret_cast<const uint16_t*>(
                                    binData.data() + bufferView["byteOffset"].toInt()
                                );
                                for (size_t i = 0; i < indexCount; i++) {
                                    skData.meshData.indices.push_back(static_cast<uint32_t>(src[i]));
                                }
                            } else if (compType == GL_UNSIGNED_INT) {
                                const uint32_t* src = reinterpret_cast<const uint32_t*>(
                                    binData.data() + bufferView["byteOffset"].toInt()
                                );
                                for (size_t i = 0; i < indexCount; i++) {
                                    skData.meshData.indices.push_back(src[i]);  // Already unsigned int
                                }
                            } else {
                                throw std::runtime_error("Unsupported component type for joints attribute");
                            }
                        }

                        if (flipWinding) {  // Flip winding for vertex indexing
                            if (!skData.meshData.indices.empty()) {
                                // If indexed flip the winding there
                                for (size_t i = 0; i < skData.meshData.indices.size(); i += 3) {
                                    std::swap(skData.meshData.indices[i + 1], skData.meshData.indices[i + 2]);
                                }
                            } else {
                                // Otherwise reorder the vertices directly
                                for (size_t i = 0; i < skData.meshData.vertices.size(); i += 3) {
                                    std::swap(skData.meshData.vertices[i + 1], skData.meshData.vertices[i + 2]);
                                }
                            }
                        }

                        // ######################### Parsing skin stuff ################################
                        const Json::JsonValue& skin = jsonChunkObj["skins"][skinIndex];

                        // Parse joint indices
                        const Json::JsonArray& joints = skin["joints"].toArray();
                        skData.jointNodeIndices.reserve(joints.size());
                        for (const Json::JsonValue& jointIdx : joints) {
                            skData.jointNodeIndices.push_back(jointIdx.toInt());
                        }

                        // Read inverse bind matrices
                        const Json::JsonValue&
                            inverseBinMatrixAccessor = jsonChunkObj["accessors"][skin["inverseBindMatrices"].toInt()];
                        compType = static_cast<unsigned int>(inverseBinMatrixAccessor["componentType"].toInt());
                        if (compType != GL_FLOAT) {
                            throw std::runtime_error("inverseBindMatrices is not of component type float");
                        } else if (inverseBinMatrixAccessor["type"].toString() != "MAT4") {
                            throw std::runtime_error("inverseBindMatrices is not of type MAT4");
                        }

                        {
                            const Json::JsonValue&
                                bufferView = jsonChunkObj["bufferViews"]
                                                         [inverseBinMatrixAccessor["bufferView"].toInt()];

                            int elementCount = inverseBinMatrixAccessor["count"].toInt();
                            skData.inverseBindMatrices.resize(elementCount);

                            const std::vector<char>& binData = binaryChunks[bufferView["buffer"].toInt()];
                            int byteLength = bufferView["byteLength"].toInt();
                            int byteOffset = bufferView["byteOffset"].toInt();

                            std::memcpy(skData.inverseBindMatrices.data(), binData.data() + byteOffset, byteLength);
                        }
                    }
                }

                nodesArray.push_back(std::move(node));
            }

            if (skData.animatedMeshNodeIndex == -1) {
                throw std::runtime_error("Could not find mesh node in glb file");
            }

            // Parse animations
            skData.animations.reserve(jsonChunkObj["animations"].toArray().size());
            for (const Json::JsonValue& animationJson : jsonChunkObj["animations"].toArray()) {
                AnimationDeclare animDeclaration;
                animDeclaration.name = animationJson["name"].toString();
                animDeclaration.channels.reserve(animationJson["channels"].toArray().size());

                for (const Json::JsonValue& channel : animationJson["channels"].toArray()) {
                    int targetNode = channel["target"]["node"].toInt();
                    const Json::JsonValue& sampler = animationJson["samplers"][channel["sampler"].toInt()];

                    // Note: Only supports LINEAR and STEP currently
                    const std::string& interpolatioString = sampler["interpolation"].toString();
                    Interpolation interpolation = interpolatioString == "LINEAR" ? Interpolation::LINEAR
                                                                                 : Interpolation::STEP;
                    if (interpolatioString != "LINEAR" && interpolatioString != "STEP") {
                        throw std::runtime_error(
                            "Currently unsupported animation interpolation: " + interpolatioString
                        );
                    }

                    const Json::JsonValue& inputAccessor = jsonChunkObj["accessors"][sampler["input"].toInt()];
                    const Json::JsonValue& inputBufferView = jsonChunkObj["bufferViews"]
                                                                         [inputAccessor["bufferView"].toInt()];

                    const Json::JsonValue& outputAccessor = jsonChunkObj["accessors"][sampler["output"].toInt()];
                    const Json::JsonValue& outputBufferView = jsonChunkObj["bufferViews"]
                                                                          [outputAccessor["bufferView"].toInt()];

                    int elementCount = inputAccessor["count"].toInt();
                    if (elementCount != outputAccessor["count"].toInt()) {
                        throw std::runtime_error("Animation input element count differs from ouput count");
                    } else if (static_cast<unsigned int>(inputAccessor["componentType"].toInt()) != GL_FLOAT) {
                        throw std::runtime_error("Animation input was not of component type float");
                    } else if (static_cast<unsigned int>(outputAccessor["componentType"].toInt()) != GL_FLOAT) {
                        throw std::runtime_error("Animation output was not of component type float");
                    } else if (inputAccessor["type"].toString() != "SCALAR") {
                        throw std::runtime_error("Animation input was not of type SCALAR");
                    }

                    const std::vector<char>& binaryChunkInput = binaryChunks[inputBufferView["buffer"].toInt()];
                    const std::vector<char>& binaryChunkOuput = binaryChunks[outputBufferView["buffer"].toInt()];
                    const float* timestamps = reinterpret_cast<const float*>(
                        binaryChunkInput.data() + inputBufferView["byteOffset"].toInt()
                    );

                    const std::string& channelString = channel["target"]["path"].toString();
                    if (channelString == "translation") {
                        if (outputAccessor["type"].toString() != "VEC3") {
                            throw std::runtime_error("Translation animation channel is not of type VEC4");
                        }

                        std::vector<Keyframe<glm::vec3>> keyframes;
                        keyframes.reserve(elementCount);

                        const glm::vec3* values = reinterpret_cast<const glm::vec3*>(
                            binaryChunkOuput.data() + outputBufferView["byteOffset"].toInt()
                        );
                        for (int i = 0; i < elementCount; i++) {
                            keyframes.push_back({timestamps[i], values[i]});
                        }

                        std::shared_ptr<Timeline<glm::vec3>> timeline = std::make_shared<Timeline<glm::vec3>>(
                            std::move(keyframes), interpolation
                        );
                        animDeclaration.channels.push_back({targetNode, AnimationProperty::Translation, timeline});
                    } else if (channelString == "rotation") {
                        if (outputAccessor["type"].toString() != "VEC4") {
                            throw std::runtime_error("Rotation animation channel is not of type VEC4");
                        }

                        std::vector<Keyframe<glm::quat>> keyframes;
                        keyframes.reserve(elementCount);

                        const glm::quat* values = reinterpret_cast<const glm::quat*>(
                            binaryChunkOuput.data() + outputBufferView["byteOffset"].toInt()
                        );
                        for (int i = 0; i < elementCount; i++) {
                            keyframes.push_back({timestamps[i], values[i]});
                        }

                        std::shared_ptr<Timeline<glm::quat>> timeline = std::make_shared<Timeline<glm::quat>>(
                            std::move(keyframes), interpolation
                        );
                        animDeclaration.channels.push_back({targetNode, AnimationProperty::Rotation, timeline});
                    } else if (channelString == "scale") {
                        if (outputAccessor["type"].toString() != "VEC3") {
                            throw std::runtime_error("Scale animation channel is not of type VEC3");
                        }

                        std::vector<Keyframe<float>> keyframes;
                        keyframes.reserve(elementCount);

                        const glm::vec3* values = reinterpret_cast<const glm::vec3*>(
                            binaryChunkOuput.data() + outputBufferView["byteOffset"].toInt()
                        );
                        for (int i = 0; i < elementCount; i++) {
                            keyframes.push_back({timestamps[i], values[i].x});  // Forcebly use uniform scale
                        }

                        std::shared_ptr<Timeline<float>> timeline = std::make_shared<Timeline<float>>(
                            std::move(keyframes), interpolation
                        );
                        animDeclaration.channels.push_back({targetNode, AnimationProperty::Scale, timeline});
                    } else {
                        throw std::runtime_error("Unknown animation path: " + channel["target"]["path"].toString());
                    }
                }

                skData.animations.push_back(std::move(animDeclaration));
            }

            // Resolve parent idices
            for (int parent = 0; parent < nodesArray.size(); parent++) {
                for (int child : nodesArray[parent].childIndices) {
                    nodesArray[child].parentIndex = parent;
                }
            }
            skData.nodeArray = std::move(nodesArray);
        } catch (const std::exception& e) {
            throw std::runtime_error("Could not process glb json chunk: " + std::string(e.what()));
        }
    }

    skData.meshData.bounds = BoundingBox::notCullable();  // TODO: Remove, this is just for testing

    return skData;
}
