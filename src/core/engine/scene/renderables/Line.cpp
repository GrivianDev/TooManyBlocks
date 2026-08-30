#include "Line.h"

#include <GL/glew.h>

#include "engine/rendering/GLUtils.h"
#include "engine/rendering/opengl/VertexArray.h"
#include "engine/rendering/opengl/VertexBuffer.h"
#include "engine/rendering/opengl/VertexBufferLayout.h"

Line::Line(const glm::vec3& start, const glm::vec3& end, float lineWidth, std::shared_ptr<Material> material)
    : Renderable(material), m_bounds({glm::min(start, end), glm::max(start, end)}), m_lineWidth(lineWidth) {
    float lineVertices[] = {start.x, start.y, start.z, end.x, end.y, end.z};

    VertexBuffer vbo = VertexBuffer::create(lineVertices, sizeof(lineVertices));
    VertexBufferLayout layout;
    layout.push(GL_FLOAT, 3);
    vbo.setLayout(layout);
    VertexArray vao = VertexArray::create();
    vao.addBuffer(vbo);

    m_data = std::make_unique<NonIndexedRenderData>(std::move(vao), std::move(vbo));
}

void Line::draw() const {
    GLCALL(glLineWidth(m_lineWidth));
    m_data->drawAs(GL_LINES);
}

BoundingBox Line::getBoundingBox() const { return m_bounds; }