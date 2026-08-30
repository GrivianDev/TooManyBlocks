#ifndef TOOMANYBLOCKS_RENDERABLE_H
#define TOOMANYBLOCKS_RENDERABLE_H

#include <memory>

#include "engine/scene/SceneComponent.h"
#include "engine/geometry/BoundingVolume.h"
#include "engine/rendering/material/Material.h"

class Renderable : public SceneComponent {
protected:
    std::shared_ptr<Material> m_material;

public:
    Renderable(std::shared_ptr<Material> material = nullptr) : m_material(material) {}
    virtual ~Renderable() = default;

    virtual void draw() const = 0;

    virtual bool isReady() const { return m_material && m_material->isReady(); }

    virtual void assignMaterial(std::shared_ptr<Material> material) { m_material = material; }

    virtual std::shared_ptr<Material> getMaterial() const { return m_material; };

    virtual BoundingBox getBoundingBox() const { return BoundingBox::invalid(); };

    virtual Transform getRenderableTransform() const { return getGlobalTransform(); }
};

#endif
