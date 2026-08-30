#ifndef TOOMANYBLOCKS_LINE_H
#define TOOMANYBLOCKS_LINE_H

#include <memory>

#include "engine/geometry/BoundingVolume.h"
#include "engine/rendering/RenderData.h"
#include "engine/scene/renderables/Renderable.h"

class Line : public Renderable {
private:
    std::unique_ptr<RenderData> m_data;
    BoundingBox m_bounds;
    float m_lineWidth;

public:
    Line(
        const glm::vec3& start,
        const glm::vec3& end,
        float lineWidth = 2.0f,
        std::shared_ptr<Material> material = nullptr
    );
    virtual ~Line() = default;

    void draw() const override;

    inline void setLineWidth(float lineWidth) { m_lineWidth = lineWidth; }

    inline float getLineWidht() const { return m_lineWidth; }

    virtual BoundingBox getBoundingBox() const override;
};

#endif