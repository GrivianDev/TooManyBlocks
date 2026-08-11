#include "SkeletalMeshBlueprint.h"

#include <GL/glew.h>

#include <memory>

#include "engine/rendering/lowlevelapi/IndexBuffer.h"
#include "engine/rendering/lowlevelapi/UniformBuffer.h"
#include "engine/rendering/lowlevelapi/VertexArray.h"
#include "engine/rendering/lowlevelapi/VertexBuffer.h"

SkeletalMesh::Asset createSkeletalMeshAsset(const CPUSkeletalMeshData& cpuSkeletalMesh) {
    VertexBuffer vbo = VertexBuffer::create(
        cpuSkeletalMesh.meshData.vertices.data(), cpuSkeletalMesh.meshData.vertices.size() * sizeof(SkeletalVertex)
    );

    VertexBufferLayout layout;
    layout.push(GL_FLOAT, 3);         // Position
    layout.push(GL_FLOAT, 2);         // UV
    layout.push(GL_FLOAT, 3);         // Normal
    layout.push(GL_UNSIGNED_INT, 4);  // Joint indices
    layout.push(GL_FLOAT, 4);         // Joint weights
    vbo.setLayout(layout);

    VertexArray vao = VertexArray::create();
    vao.addBuffer(vbo);

    std::unique_ptr<RenderData> renderData;
    if (cpuSkeletalMesh.meshData.isIndexed()) {
        IndexBuffer ibo = IndexBuffer::create(
            cpuSkeletalMesh.meshData.indices.data(), cpuSkeletalMesh.meshData.indices.size()
        );
        renderData = std::make_unique<IndexedRenderData>(std::move(vao), std::move(vbo), std::move(ibo));
    } else {
        renderData = std::make_unique<NonIndexedRenderData>(std::move(vao), std::move(vbo));
    }

    UniformBuffer inverseBindMatricesUBO = UniformBuffer::create(
        cpuSkeletalMesh.inverseBindMatrices.data(), cpuSkeletalMesh.inverseBindMatrices.size() * sizeof(glm::mat4)
    );

    return SkeletalMesh::Asset{
        std::move(renderData),
        std::move(inverseBindMatricesUBO),
        cpuSkeletalMesh.inverseBindMatrices,
        cpuSkeletalMesh.jointNodeIndices,
        cpuSkeletalMesh.nodeArray,
        cpuSkeletalMesh.animatedMeshNodeIndex,
        cpuSkeletalMesh.animations,
        cpuSkeletalMesh.meshData.bounds
    };
}
