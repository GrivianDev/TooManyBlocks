#include "SkeletalMesh.h"

#include <GL/glew.h>

SkeletalMesh::SkeletalMesh(const Future<Asset>& asset, std::shared_ptr<Material> material)
    : Renderable(material), m_asset(asset) {
    m_instance = Future<Instance>(
        [this, asset]() {
            const Asset& assetVal = asset.value();
            std::vector<SceneComponent> sceneCompArray(assetVal.nodeArray.size());
            for (size_t i = 0; i < sceneCompArray.size(); i++) {
                sceneCompArray[i].getLocalTransform() = assetVal.nodeArray[i].localTransform;
                sceneCompArray[i].setName(assetVal.nodeArray[i].name);
                for (int childIndex : assetVal.nodeArray[i].childIndices) {
                    sceneCompArray[assetVal.nodeArray[childIndex].parentIndex].attachChild(&sceneCompArray[childIndex]);
                }
                if (assetVal.nodeArray[i].parentIndex < 0) {
                    attachChild(&sceneCompArray[i], AttachRule::None);
                }
            }

            std::vector<glm::mat4> initalMatrices(std::max<int>(assetVal.jointNodeIndices.size(), 4), glm::mat4(1.0f));

            return Instance{
                std::move(sceneCompArray),
                UniformBuffer::create(initalMatrices.data(), initalMatrices.size() * sizeof(glm::mat4))
            };
        },
        DEFAULT_TASKCONTEXT,
        Executor::Main
    );

    m_instance.dependsOn(asset).start();
}

void SkeletalMesh::draw() const {
    if (!isReady()) return;

    m_asset.value().meshData->drawAs(GL_TRIANGLES);
}

const AnimationClip* SkeletalMesh::getAnimation(const std::string& name) const {
    if (!isReady()) return nullptr;

    for (const AnimationClip& clip : m_asset.value().animations) {
        if (clip.getName() == name) {
            return &clip;
        }
    }
    return nullptr;
}

const UniformBuffer* SkeletalMesh::getJointMatrices() const {
    if (!isReady()) return nullptr;

    const std::vector<int>& jointNodeIndices = m_asset.value().jointNodeIndices;

    std::vector<glm::mat4> jointMatrices;
    jointMatrices.reserve(jointNodeIndices.size());

    for (int i = 0; i < jointNodeIndices.size(); i++) {
        int jointIdx = jointNodeIndices[i];
        const SceneComponent& joint = m_instance.value().nodeArray[jointIdx];
        const glm::mat4& bindMatrix = m_asset.value().inverseBindMatrices[i];

        jointMatrices.push_back(joint.getGlobalTransform().getModelMatrix() * bindMatrix);
    }
    m_instance.value().jointMatricesUBO.updateData(jointMatrices.data(), jointMatrices.size() * sizeof(glm::mat4));
    return &m_instance.value().jointMatricesUBO;
}

size_t SkeletalMesh::getNodeCount() const {
    if (!isReady()) return 0;

    return m_instance.value().nodeArray.size();
}

BoundingBox SkeletalMesh::getBoundingBox() const {
    if (!m_asset.isReady()) return Renderable::getBoundingBox();

    return m_asset.value().bounds;
}

void SkeletalMesh::update(float deltaTime) {
    if (m_animController && isReady()) {
        m_animController->update(deltaTime);

        const std::vector<Transform>& animatedTransform = m_animController->getEvaluationTransforms();
        for (size_t i = 0; i < animatedTransform.size(); i++) {
            m_instance.value().nodeArray[i].getLocalTransform() = animatedTransform[i];
        }
    }
}