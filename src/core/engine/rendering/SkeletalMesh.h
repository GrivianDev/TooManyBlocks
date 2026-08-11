#ifndef TOOMANYBLOCKS_SKELETALMESH_H
#define TOOMANYBLOCKS_SKELETALMESH_H

#include <memory>

#include "engine/animation/Animation.h"
#include "engine/assets/cpu/CPUSkeletalMeshData.h"
#include "engine/geometry/BoundingVolume.h"
#include "engine/rendering/RenderData.h"
#include "engine/rendering/Renderable.h"
#include "engine/rendering/lowlevelapi/UniformBuffer.h"
#include "foundation/threading/Future.h"

class SkeletalMesh : public Renderable, public Updatable {
public:
    struct Asset {
        std::unique_ptr<RenderData> meshData;

        UniformBuffer inverseBindMatricesUBO;
        std::vector<glm::mat4> inverseBindMatrices;
        std::vector<int> jointNodeIndices;  // indexed by joint index (Needed to build joint matrices)

        std::vector<Node> nodeArray;
        int animatedMeshNodeIndex;
        std::vector<AnimationDeclare> animations;
        BoundingBox bounds;
    };

    struct Instance {
        std::vector<SceneComponent> nodeArray;
        std::vector<Animation> animations;
        UniformBuffer jointMatricesUBO;
    };

private:
    Future<Asset> m_asset;
    Future<Instance> m_instance;
    Animation* m_activeAnim;

public:
    SkeletalMesh() : m_activeAnim(nullptr) {}
    SkeletalMesh(const Future<Asset>& asset, std::shared_ptr<Material> material = nullptr);
    virtual ~SkeletalMesh() = default;

    void draw() const override;

    bool playAnimation(const std::string& animation, bool loop = false, bool restart = true);

    void stopAnimation();

    inline bool isReady() const override { return Renderable::isReady() && m_asset.isReady() && m_instance.isReady(); }

    inline const Animation* getActiveAnimation() const { return m_activeAnim; }

    inline Future<Asset>& getAssetHandle() { return m_asset; }

    const UniformBuffer* getJointMatrices() const;

    Transform getRenderableTransform() const override;

    virtual BoundingBox getBoundingBox() const override;

    void update(float deltaTime) override;
};

#endif
