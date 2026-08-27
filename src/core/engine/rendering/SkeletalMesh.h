#ifndef TOOMANYBLOCKS_SKELETALMESH_H
#define TOOMANYBLOCKS_SKELETALMESH_H

#include <memory>

#include "engine/animation/AnimationClip.h"
#include "engine/animation/AnimationController.h"
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
        std::vector<AnimationClip> animations;
        BoundingBox bounds;
    };

    struct Instance {
        std::vector<SceneComponent> nodeArray;
        UniformBuffer jointMatricesUBO;
    };

private:
    Future<Asset> m_asset;
    Future<Instance> m_instance;
    std::unique_ptr<AnimationController> m_animController;

public:
    float testValue = 0.0f;

    SkeletalMesh() = default;
    SkeletalMesh(const Future<Asset>& asset, std::shared_ptr<Material> material = nullptr);
    virtual ~SkeletalMesh() = default;

    void draw() const override;

    inline bool isReady() const override { return Renderable::isReady() && m_asset.isReady() && m_instance.isReady(); }

    inline void setAnimationController(std::unique_ptr<AnimationController> controller) {
        m_animController = std::move(controller);
    }

    inline const AnimationController* getAnimationController() const { return m_animController.get(); }

    const AnimationClip* getAnimation(const std::string& name) const;

    inline Future<Asset>& getAssetHandle() { return m_asset; }

    const UniformBuffer* getJointMatrices() const;

    size_t getNodeCount() const;

    virtual BoundingBox getBoundingBox() const override;

    void update(float deltaTime) override;
};

#endif
