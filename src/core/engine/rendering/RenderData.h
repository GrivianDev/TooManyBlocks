#ifndef TOOMANYBLOCKS_RENDERDATA_H
#define TOOMANYBLOCKS_RENDERDATA_H

#include "engine/geometry/BoundingVolume.h"
#include "engine/rendering/opengl/IndexBuffer.h"
#include "engine/rendering/opengl/VertexArray.h"
#include "engine/rendering/opengl/VertexBuffer.h"

class RenderData {
public:
    virtual ~RenderData() = default;

    virtual void drawAs(unsigned int type) const = 0;
};

class NonIndexedRenderData : public RenderData {
protected:
    VertexArray m_vao;
    VertexBuffer m_vbo;

public:
    NonIndexedRenderData(VertexArray&& vao, VertexBuffer&& vbo) : m_vao(std::move(vao)), m_vbo(std::move(vbo)) {}
    virtual ~NonIndexedRenderData() = default;

    virtual void drawAs(unsigned int type) const override;
};

class IndexedRenderData : public NonIndexedRenderData {
protected:
    IndexBuffer m_ibo;

public:
    IndexedRenderData(VertexArray&& vao, VertexBuffer&& vbo, IndexBuffer&& ibo)
        : NonIndexedRenderData(std::move(vao), std::move(vbo)), m_ibo(std::move(ibo)) {}
    virtual ~IndexedRenderData() = default;

    virtual void drawAs(unsigned int type) const override;
};

#endif