#include "SceneComponent.h"

#include <algorithm>

#include "engine/Scene.h"

SceneComponent::SceneComponent() : scene(nullptr), parent(nullptr), m_attachRule(AttachRule::None), m_visible(true) {};

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
    if (child->parent != this) {
        child->parent = this;
        children.push_back(child);
    }
    child->m_attachRule = rule;
}

void SceneComponent::detachChild(SceneComponent* child) {
    auto it = std::find(children.begin(), children.end(), child);
    if (it != children.end()) {
        children.erase(it);
    }
    child->parent = nullptr;
}

void SceneComponent::detachAll() {
    for (SceneComponent* childPtr : children) {
        childPtr->parent = nullptr;
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

Transform SceneComponent::getGlobalTransform() const {
    if (parent && m_attachRule != AttachRule::None) {
        Transform parentTransform = parent->getGlobalTransform();
        Transform result;

        switch (m_attachRule) {
            case AttachRule::Full: return parentTransform * m_transform;

            case AttachRule::PosAndScale:
                result.setPosition(
                    parentTransform.getPosition() + (m_transform.getPosition() * parentTransform.getScale())
                );                                                  // Adjust based on parent position / scale
                result.setRotation(m_transform.getRotationQuat());  // No rotation inheritance
                result.setScale(parentTransform.getScale() * m_transform.getScale());
                return result;

            case AttachRule::PosAndRot:
                result.setPosition(
                    parentTransform.getPosition() + (parentTransform.getRotationQuat() * m_transform.getPosition())
                );  // Adjust based on parent position / rotation
                result.setRotation(parentTransform.getRotationQuat() * m_transform.getRotationQuat());
                result.setScale(m_transform.getScale());  // No scale inheritance
                return result;

            case AttachRule::RotAndScale:
                result.setPosition(m_transform.getPosition());  // No position inheritance
                result.setRotation(parentTransform.getRotationQuat() * m_transform.getRotationQuat());
                result.setScale(parentTransform.getScale() * m_transform.getScale());
                return result;

            case AttachRule::PosOnly:
                result.setPosition(
                    parentTransform.getPosition() + m_transform.getPosition()
                );  // Only apply parent translation
                result.setRotation(m_transform.getRotationQuat());
                result.setScale(m_transform.getScale());
                return result;

            case AttachRule::RotOnly:
                result.setPosition(m_transform.getPosition());
                result.setRotation(
                    parentTransform.getRotationQuat() * m_transform.getRotationQuat()
                );  // Only apply parent rotation
                result.setScale(m_transform.getScale());
                return result;

            case AttachRule::ScaleOnly:
                result.setPosition(m_transform.getPosition());
                result.setRotation(m_transform.getRotationQuat());
                result.setScale(parentTransform.getScale() * m_transform.getScale());  // Only apply parent scale
                return result;
            default: return m_transform;
        }
    } else {
        return m_transform;
    }
}