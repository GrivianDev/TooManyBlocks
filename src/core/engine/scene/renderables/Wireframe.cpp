#include "Wireframe.h"

#include <GL/glew.h>

#include "engine/rendering/GLUtils.h"

Wireframe Wireframe::fromBoundigBox(const BoundingBox& bbox) {
    glm::vec3 corners[8] = {
        {bbox.min.x, bbox.min.y, bbox.min.z},  // 0
        {bbox.max.x, bbox.min.y, bbox.min.z},  // 1
        {bbox.max.x, bbox.max.y, bbox.min.z},  // 2
        {bbox.min.x, bbox.max.y, bbox.min.z},  // 3
        {bbox.min.x, bbox.min.y, bbox.max.z},  // 4
        {bbox.max.x, bbox.min.y, bbox.max.z},  // 5
        {bbox.max.x, bbox.max.y, bbox.max.z},  // 6
        {bbox.min.x, bbox.max.y, bbox.max.z},  // 7
    };

    const unsigned int indices[24] = {
        0, 1, 1, 2, 2, 3, 3, 0,  // bottom
        4, 5, 5, 6, 6, 7, 7, 4,  // top
        0, 4, 1, 5, 2, 6, 3, 7   // verticals
    };

    VertexBuffer vbo = VertexBuffer::create(&corners[0], 8 * sizeof(glm::vec3));

    VertexBufferLayout layout;
    layout.push(GL_FLOAT, 3);
    vbo.setLayout(layout);

    VertexArray vao = VertexArray::create();
    vao.addBuffer(vbo);

    IndexBuffer ibo = IndexBuffer::create(indices, 24);

    std::shared_ptr<IndexedRenderData> renderData = std::make_shared<IndexedRenderData>(
        std::move(vao), std::move(vbo), std::move(ibo)
    );

    return Wireframe(renderData, bbox);
}

Wireframe Wireframe::fromFrustum(const glm::mat4& viewProj) {
    const glm::mat4 invViewProj = glm::inverse(viewProj);

    // OpenGL NDC: x/y/z are in [-1, 1]
    glm::vec3 corners[8] = {
        {-1.0f, -1.0f, -1.0f},  // 0 near bottom-left
        {1.0f, -1.0f, -1.0f},   // 1 near bottom-right
        {1.0f, 1.0f, -1.0f},    // 2 near top-right
        {-1.0f, 1.0f, -1.0f},   // 3 near top-left

        {-1.0f, -1.0f, 1.0f},  // 4 far bottom-left
        {1.0f, -1.0f, 1.0f},   // 5 far bottom-right
        {1.0f, 1.0f, 1.0f},    // 6 far top-right
        {-1.0f, 1.0f, 1.0f},   // 7 far top-left
    };

    for (glm::vec3& corner : corners) {
        glm::vec4 world = invViewProj * glm::vec4(corner, 1.0f);
        corner = glm::vec3(world) / world.w;
    }

    const unsigned int indices[24] = {
        0, 1, 1, 2, 2, 3, 3, 0,  // near
        4, 5, 5, 6, 6, 7, 7, 4,  // far
        0, 4, 1, 5, 2, 6, 3, 7   // sides
    };

    VertexBuffer vbo = VertexBuffer::create(corners, 8 * sizeof(glm::vec3));

    VertexBufferLayout layout;
    layout.push(GL_FLOAT, 3);
    vbo.setLayout(layout);

    VertexArray vao = VertexArray::create();
    vao.addBuffer(vbo);

    IndexBuffer ibo = IndexBuffer::create(indices, 24);

    std::shared_ptr<IndexedRenderData> renderData = std::make_shared<IndexedRenderData>(
        std::move(vao), std::move(vbo), std::move(ibo)
    );

    // Calculate an AABB around the frustum.
    BoundingBox bbox{corners[0], corners[0]};
    for (int i = 1; i < 8; i++) {
        bbox.min = glm::min(bbox.min, corners[i]);
        bbox.max = glm::max(bbox.max, corners[i]);
    }

    return Wireframe(renderData, bbox);
}

Wireframe::Wireframe(
    std::shared_ptr<RenderData> renderData,
    const BoundingBox& bounds,
    float lineWidth,
    std::shared_ptr<Material> material
)
    : Renderable(material), m_data(renderData), m_lineWidth(lineWidth) {
    setLocalBounds(bounds);
}

void Wireframe::draw() const {
    GLCALL(glLineWidth(m_lineWidth));
    m_data->drawAs(GL_LINES);
}
