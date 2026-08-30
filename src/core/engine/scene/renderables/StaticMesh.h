#ifndef TOOMANYBLOCKS_STATICMESH_H
#define TOOMANYBLOCKS_STATICMESH_H

#include <memory>

#include "engine/geometry/BoundingVolume.h"
#include "engine/rendering/RenderData.h"
#include "engine/scene/renderables/Renderable.h"
#include "foundation/threading/Future.h"

class StaticMesh : public Renderable {
public:
    struct Asset {
        std::unique_ptr<RenderData> renderData;
        BoundingBox bounds;
    };

private:
    Future<Asset> m_asset;

public:
    StaticMesh() = default;
    StaticMesh(const Future<Asset>& asset, std::shared_ptr<Material> material = nullptr)
        : Renderable(material), m_asset(asset) {}
    virtual ~StaticMesh() = default;

    void draw() const override;

    inline bool isReady() const override { return Renderable::isReady() && m_asset.isReady(); }

    inline Future<Asset>& getAssetHandle() { return m_asset; }

    virtual BoundingBox getBoundingBox() const override;
};

#endif
