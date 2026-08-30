#include "ChunkMeshCreate.h"

#include <GL/glew.h>

#include <memory>

#include "engine/rendering/opengl/IndexBuffer.h"
#include "engine/rendering/opengl/VertexArray.h"
#include "engine/rendering/opengl/VertexBuffer.h"
#include "engine/rendering/opengl/VertexBufferLayout.h"

StaticMesh::Asset createStaticMeshAsset(const CPURenderData<CompactChunkVertex>& cpuStaticMesh) {
    VertexBuffer vbo = VertexBuffer::create(
        cpuStaticMesh.vertices.data(), cpuStaticMesh.vertices.size() * sizeof(CompactChunkVertex)
    );

    VertexBufferLayout layout;
    // Compressed data
    layout.push(GL_UNSIGNED_INT, 1);
    layout.push(GL_UNSIGNED_INT, 1);
    vbo.setLayout(layout);

    VertexArray vao = VertexArray::create();
    vao.addBuffer(vbo);

    if (cpuStaticMesh.isIndexed()) {
        IndexBuffer ibo = IndexBuffer::create(cpuStaticMesh.indices.data(), cpuStaticMesh.indices.size());
        std::unique_ptr<RenderData> renderData = std::make_unique<IndexedRenderData>(
            std::move(vao), std::move(vbo), std::move(ibo)
        );
        return StaticMesh::Asset{std::move(renderData), cpuStaticMesh.bounds};
    } else {
        std::unique_ptr<RenderData> renderData = std::make_unique<NonIndexedRenderData>(std::move(vao), std::move(vbo));
        return StaticMesh::Asset{std::move(renderData), cpuStaticMesh.bounds};
    }
}
