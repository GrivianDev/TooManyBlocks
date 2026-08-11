#include "StaticMeshBlueprint.h"

#include <GL/glew.h>

#include <memory>

StaticMesh::Asset createStaticMeshAsset(const CPURenderData<Vertex>& cpuStaticMesh) {
    VertexBuffer vbo = VertexBuffer::create(
        cpuStaticMesh.vertices.data(), cpuStaticMesh.vertices.size() * sizeof(Vertex)
    );

    VertexBufferLayout layout;
    layout.push(GL_FLOAT, 3);  // Position
    layout.push(GL_FLOAT, 2);  // UV
    layout.push(GL_FLOAT, 3);  // Normal
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
