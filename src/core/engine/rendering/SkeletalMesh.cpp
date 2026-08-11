#include "SkeletalMesh.h"

#include <GL/glew.h>

SkeletalMesh::SkeletalMesh(const Future<Asset>& asset, std::shared_ptr<Material> material)
    : Renderable(material), m_asset(asset), m_activeAnim(nullptr) {
    m_instance = Future<Instance>(
        [asset]() {
            const Asset& assetVal = asset.value();
            std::vector<SceneComponent> sceneCompArray(assetVal.nodeArray.size());
            for (size_t i = 0; i < sceneCompArray.size(); i++) {
                sceneCompArray[i].getLocalTransform() = assetVal.nodeArray[i].localTransform;
                for (int childIndex : assetVal.nodeArray[i].childIndices) {
                    sceneCompArray[assetVal.nodeArray[childIndex].parentIndex].attachChild(&sceneCompArray[childIndex]);
                }
            }

            std::vector<glm::mat4> initalMatrices(std::max<int>(assetVal.jointNodeIndices.size(), 4));
            for (size_t i = 0; i < initalMatrices.size(); i++) {
                initalMatrices[i] = glm::mat4(1.0f);
            }

            std::vector<Animation> animInstances;
            animInstances.reserve(assetVal.animations.size());

            for (const AnimationDeclare& anim : assetVal.animations) {
                Animation animInstance(anim.name);
                for (const ChannelDeclare& channelDecl : anim.channels) {
                    if (channelDecl.property == AnimationProperty::Translation) {
                        animInstance.addTranslationChannel(
                            &sceneCompArray[channelDecl.targetNodeIndex],
                            std::static_pointer_cast<Timeline<glm::vec3>>(channelDecl.timeline)
                        );
                    } else if (channelDecl.property == AnimationProperty::Rotation) {
                        animInstance.addRotationChannel(
                            &sceneCompArray[channelDecl.targetNodeIndex],
                            std::static_pointer_cast<Timeline<glm::quat>>(channelDecl.timeline)
                        );
                    } else if (channelDecl.property == AnimationProperty::Scale) {
                        animInstance.addScaleChannel(
                            &sceneCompArray[channelDecl.targetNodeIndex],
                            std::static_pointer_cast<Timeline<float>>(channelDecl.timeline)
                        );
                    }
                }
                animInstances.push_back(std::move(animInstance));
            }

            return Instance{
                std::move(sceneCompArray),
                std::move(animInstances),
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

bool SkeletalMesh::playAnimation(const std::string& animName, bool loop, bool restart) {
    if (!isReady()) return false;

    for (Animation& anim : m_instance.value().animations) {
        if (anim.getName() == animName) {
            if (restart) {
                anim.reset();
            }
            anim.setLooping(loop);
            m_activeAnim = &anim;
            return true;
        }
    }
    return false;
}

void SkeletalMesh::stopAnimation() { m_activeAnim = nullptr; }

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
    m_instance.value().jointMatricesUBO.updateData(
        jointMatrices.data(), jointMatrices.size() * sizeof(glm::mat4)
    );
    return &m_instance.value().jointMatricesUBO;
}

Transform SkeletalMesh::getRenderableTransform() const {
    if (!isReady()) return Renderable::getRenderableTransform();

    int animatedNodeIndex = m_asset.value().animatedMeshNodeIndex;
    const SceneComponent& animatedNode = m_instance.value().nodeArray[animatedNodeIndex];
    return animatedNode.getGlobalTransform() * getGlobalTransform();
}

BoundingBox SkeletalMesh::getBoundingBox() const {
    if (!m_asset.isReady()) return Renderable::getBoundingBox();

    return m_asset.value().bounds;
}

void SkeletalMesh::update(float deltaTime) {
    if (m_activeAnim) {
        m_activeAnim->update(deltaTime);
    }
}