#include "SceneComponent.h"

#include <algorithm>

#include "engine/scene/Scene.h"

void SceneComponent::markWorldStateDirty() const {
    if (m_worldStateDirty) return;

    m_worldStateDirty = true;

    for (const SceneComponent* child : children) {
        child->markWorldStateDirty();
    }
}

void SceneComponent::updateWorldState() const {
    if (!m_worldStateDirty) return;

    if (parent && m_attachRule != AttachRule::None) {
        Transform parentTransform = parent->getGlobalTransform();
        Transform result;

        switch (m_attachRule) {
            case AttachRule::Full: result = parentTransform * m_localTransform; break;

            case AttachRule::PosAndScale:
                result.setPosition(
                    parentTransform.getPosition() + (m_localTransform.getPosition() * parentTransform.getScale())
                );                                                  // Adjust based on parent position / scale
                result.setRotation(m_localTransform.getRotationQuat());  // No rotation inheritance
                result.setScale(parentTransform.getScale() * m_localTransform.getScale());
                break;

            case AttachRule::PosAndRot:
                result.setPosition(
                    parentTransform.getPosition() + (parentTransform.getRotationQuat() * m_localTransform.getPosition())
                );  // Adjust based on parent position / rotation
                result.setRotation(parentTransform.getRotationQuat() * m_localTransform.getRotationQuat());
                result.setScale(m_localTransform.getScale());  // No scale inheritance
                break;

            case AttachRule::RotAndScale:
                result.setPosition(m_localTransform.getPosition());  // No position inheritance
                result.setRotation(parentTransform.getRotationQuat() * m_localTransform.getRotationQuat());
                result.setScale(parentTransform.getScale() * m_localTransform.getScale());
                break;

            case AttachRule::PosOnly:
                result.setPosition(
                    parentTransform.getPosition() + m_localTransform.getPosition()
                );  // Only apply parent translation
                result.setRotation(m_localTransform.getRotationQuat());
                result.setScale(m_localTransform.getScale());
                break;

            case AttachRule::RotOnly:
                result.setPosition(m_localTransform.getPosition());
                result.setRotation(
                    parentTransform.getRotationQuat() * m_localTransform.getRotationQuat()
                );  // Only apply parent rotation
                result.setScale(m_localTransform.getScale());
                break;

            case AttachRule::ScaleOnly:
                result.setPosition(m_localTransform.getPosition());
                result.setRotation(m_localTransform.getRotationQuat());
                result.setScale(parentTransform.getScale() * m_localTransform.getScale());  // Only apply parent scale
                break;

            default: result = m_localTransform; break;
        }
        m_globalTransform = result;
    } else {
        m_globalTransform = m_localTransform;
    }

    updateBounds();

    m_worldStateDirty = false;
}

void SceneComponent::updateBounds() const {
    if (m_localBounds.isInvalid() || m_localBounds.isNotCullable()) {
        m_globalBounds = m_localBounds;
        return;
    }

    const glm::vec3 localCenter = (m_localBounds.min + m_localBounds.max) * 0.5f;
    const glm::vec3 localExtents = (m_localBounds.max - m_localBounds.min) * 0.5f;

    const glm::vec3 worldCenter = m_globalTransform.getPosition() +
                                  m_globalTransform.getRotationQuat() * (localCenter * m_globalTransform.getScale());

    const glm::mat3 rotation = glm::mat3_cast(m_globalTransform.getRotationQuat());
    const glm::mat3 absRotation(glm::abs(rotation[0]), glm::abs(rotation[1]), glm::abs(rotation[2]));

    const glm::vec3 worldExtents = absRotation * (localExtents * m_globalTransform.getScale());

    m_globalBounds = {worldCenter - worldExtents, worldCenter + worldExtents};
}

SceneComponent::SceneComponent()
    : scene(nullptr),
      parent(nullptr),
      m_attachRule(AttachRule::None),
      m_localBounds(BoundingBox::invalid()),
      m_globalBounds(BoundingBox::invalid()),
      m_worldStateDirty(true),
      m_visible(true) {};

SceneComponent::~SceneComponent() {
    if (parent) {
        parent->detachChild(this);
    }
    detachAll();
}

void SceneComponent::setName(const std::string& name) {
    const std::string oldName = m_name;
    m_name = name;

    if (oldName != m_name && scene) {
        scene->onNameChanged(this, oldName, m_name);
    }
}

void SceneComponent::addTag(const std::string& tag) {
    auto [it, inserted] = m_tags.insert(tag);
    if (inserted && scene) {
        scene->onTagAdded(this, tag);
    }
}

void SceneComponent::removeTag(const std::string& tag) {
    if (m_tags.erase(tag) > 0 && scene) {
        scene->onTagRemoved(this, tag);
    }
}

void SceneComponent::attachChild(SceneComponent* child, AttachRule rule) {
    for (const SceneComponent* ancestor = this; ancestor; ancestor = ancestor->parent) {
        if (ancestor == child) throw std::runtime_error("SceneComponent attachment would create a cycle");
    }
    if (child->parent == this) {
        child->m_attachRule = rule;
        child->markWorldStateDirty();
        return;
    }

    if (child->parent) {
        child->parent->detachChild(child);
    }

    child->parent = this;
    child->m_attachRule = rule;
    children.push_back(child);

    child->markWorldStateDirty();
}

void SceneComponent::detachChild(SceneComponent* child) {
    auto it = std::find(children.begin(), children.end(), child);
    if (it == children.end()) return;

    children.erase(it);
    child->parent = nullptr;
    child->markWorldStateDirty();
}

void SceneComponent::detachAll() {
    for (SceneComponent* child : children) {
        child->parent = nullptr;
        child->markWorldStateDirty();
    }
    children.clear();
}

SceneComponent* SceneComponent::findChildByName(const std::string& name) const {
    for (SceneComponent* child : children) {
        if (child->getName() == name) return child;
        if (SceneComponent* result = child->findChildByName(name)) return result;
    }

    return nullptr;
}

SceneComponent* SceneComponent::findChildByTag(const std::string& tag) const {
    for (SceneComponent* child : children) {
        if (child->hasTag(tag)) return child;
        if (SceneComponent* result = child->findChildByTag(tag)) return result;
    }

    return nullptr;
}

std::unordered_set<SceneComponent*> SceneComponent::findChildrenByName(const std::string& name) const {
    std::unordered_set<SceneComponent*> result;
    for (SceneComponent* child : children) {
        if (child->getName() == name) result.insert(child);

        std::unordered_set<SceneComponent*> childResults = child->findChildrenByName(name);
        result.insert(childResults.begin(), childResults.end());
    }

    return result;
}

std::unordered_set<SceneComponent*> SceneComponent::findChildrenByTag(const std::string& tag) const {
    std::unordered_set<SceneComponent*> result;
    for (SceneComponent* child : children) {
        if (child->hasTag(tag)) result.insert(child);

        std::unordered_set<SceneComponent*> childResults = child->findChildrenByTag(tag);
        result.insert(childResults.begin(), childResults.end());
    }

    return result;
}

void SceneComponent::setLocalTransform(const Transform& transform) {
    m_localTransform = transform;
    markWorldStateDirty();
}

void SceneComponent::setLocalBounds(const BoundingBox& bounds) {
    m_localBounds = bounds;
    markWorldStateDirty();
}

const Transform& SceneComponent::getGlobalTransform() const {
    updateWorldState();
    return m_globalTransform;
}

const BoundingBox& SceneComponent::getGlobalBounds() const {
    updateWorldState();
    return m_globalBounds;
}
