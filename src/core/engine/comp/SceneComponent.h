#ifndef TOOMANYBLOCKS_SCENECOMPONENT_H
#define TOOMANYBLOCKS_SCENECOMPONENT_H

#include <string>
#include <unordered_set>
#include <vector>

#include "datatypes/Transform.h"

class Scene;

enum class AttachRule {
    Full,
    PosAndScale,
    PosAndRot,
    RotAndScale,
    PosOnly,
    RotOnly,
    ScaleOnly,
    None
};

class SceneComponent {
    friend class Scene;

private:
    inline void setScene(Scene* scene) { this->scene = scene; }

protected:
    std::string m_name;
    std::unordered_set<std::string> m_tags;

    Scene* scene;

    SceneComponent* parent;
    std::vector<SceneComponent*> children;
    Transform m_transform;
    AttachRule m_attachRule;

    bool m_visible;

public:
    SceneComponent();
    virtual ~SceneComponent();

    void setName(const std::string& name);
    inline const std::string& getName() const { return m_name; }

    void addTag(const std::string& tag);
    void removeTag(const std::string& tag);
    inline bool hasTag(const std::string& tag) const { return m_tags.find(tag) != m_tags.end(); }
    inline const std::unordered_set<std::string>& getTags() const { return m_tags; }

    void attachChild(SceneComponent* child, AttachRule rule = AttachRule::Full);
    void detachChild(SceneComponent* child);
    void detachAll();
    inline const std::vector<SceneComponent*>& getChildren() const { return children; }

    SceneComponent* findChildByName(const std::string& name) const;
    SceneComponent* findChildByTag(const std::string& tag) const;

    template <typename T>
    T* findChildByType() const {
        static_assert(std::is_base_of<SceneComponent, T>::value, "T must derive from SceneComponent");

        for (SceneComponent* child : children) {
            if (T* result = dynamic_cast<T*>(child)) return result;
            if (T* result = child->findChildByType<T>()) return result;
        }

        return nullptr;
    }

    std::unordered_set<SceneComponent*> findChildrenByName(const std::string& name) const;
    std::unordered_set<SceneComponent*> findChildrenByTag(const std::string& tag) const;

    template <typename T>
    std::unordered_set<T*> findChildrenByType() const {
        static_assert(std::is_base_of<SceneComponent, T>::value, "T must derive from SceneComponent");

        std::unordered_set<T*> result;
        for (SceneComponent* child : children) {
            if (T* component = dynamic_cast<T*>(child)) result.insert(component);

            std::unordered_set<T*> childResults = child->findChildrenByType<T>();
            result.insert(childResults.begin(), childResults.end());
        }

        return result;
    }

    inline AttachRule getAttachRule() const { return m_attachRule; }
    inline Transform& getLocalTransform() { return m_transform; }
    Transform getGlobalTransform() const;

    inline void setVisible(bool visible) { m_visible = visible; }
    inline bool isVisible() const { return m_visible; }
};

#endif
