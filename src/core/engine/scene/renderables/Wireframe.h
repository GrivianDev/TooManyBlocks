#ifndef TOOMANYBLOCKS_WIREFRAME_H
#define TOOMANYBLOCKS_WIREFRAME_H

#include <glm/glm.hpp>
#include <memory>

#include "engine/rendering/RenderData.h"
#include "engine/scene/renderables/Renderable.h"

class Wireframe : public Renderable {
private:
    std::shared_ptr<RenderData> m_data;
    float m_lineWidth;

public:
    static Wireframe fromBoundigBox(const BoundingBox& bbox);

    static Wireframe fromFrustum(const glm::mat4& viewProj);

    Wireframe(
        std::shared_ptr<RenderData> renderData,
        const BoundingBox& bounds,
        float lineWidth = 2.0f,
        std::shared_ptr<Material> material = nullptr
    );
    virtual ~Wireframe() = default;

    void draw() const override;

    inline GeometryType geometryType() const override { return GeometryType::Line; }

    inline void setLineWidth(float lineWidth) { m_lineWidth = lineWidth; }

    inline float getLineWidht() const { return m_lineWidth; }
};

#endif
