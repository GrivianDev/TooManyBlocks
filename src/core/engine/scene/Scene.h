#ifndef TOOMANYBLOCKS_SCENE_H
#define TOOMANYBLOCKS_SCENE_H

#include <cinttypes>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "engine/Updatable.h"
#include "engine/scene/SceneComponent.h"
#include "engine/scene/lights/Light.h"
#include "engine/scene/renderables/Renderable.h"

class Scene : public Updatable {
    friend class SceneComponent;

private:
    std::unordered_map<SceneComponent*, std::unique_ptr<SceneComponent>> m_allSceneObjects;

    std::unordered_map<std::type_index, unsigned int> m_typeCounter;
    std::unordered_map<std::string, std::unordered_set<SceneComponent*>> m_nameIndex;
    std::unordered_map<std::string, std::unordered_set<SceneComponent*>> m_tagIndex;
    std::unordered_map<std::type_index, std::unordered_set<SceneComponent*>> m_typeIndex;

    std::vector<Light*> m_lights;
    std::vector<Updatable*> m_updatables;
    std::vector<Renderable*> m_renderables;

    void onNameChanged(SceneComponent* component, const std::string& oldName, const std::string& newName);

    void onTagAdded(SceneComponent* component, const std::string& tag);

    void onTagRemoved(SceneComponent* component, const std::string& tag);

public:
    template <typename T, typename... Args>
    T* create(Args&&... args) {
        static_assert(std::is_base_of<SceneComponent, T>::value, "T must derive from SceneComponent");
        std::unique_ptr<T> object = std::make_unique<T>(std::forward<Args>(args)...);

        T* ptr = object.get();
        ptr->setScene(this);
        m_allSceneObjects.emplace(ptr, std::move(object));

        const std::type_index index = typeid(T);
        m_typeIndex[index].insert(ptr);
        ptr->setName(std::string(index.name()) + "_" + std::to_string(m_typeCounter[index]++));

        // Register specific types for fast iterations
        if (Light* light = dynamic_cast<Light*>(ptr)) {
            m_lights.push_back(light);
        }
        if (Updatable* updatable = dynamic_cast<Updatable*>(ptr)) {
            m_updatables.push_back(updatable);
        }
        if (Renderable* renderable = dynamic_cast<Renderable*>(ptr)) {
            m_renderables.push_back(renderable);
        }

        return ptr;
    }

    void destroy(SceneComponent* component);

    // Queries
    SceneComponent* findByName(const std::string& name) const;
    SceneComponent* findByTag(const std::string& tag) const;
    template <typename T>
    T* findByType() const {
        static_assert(std::is_base_of<SceneComponent, T>::value, "T must derive from SceneComponent");
        auto it = m_typeIndex.find(typeid(T));
        if (it == m_typeIndex.end() || it->second.empty()) return nullptr;
        return static_cast<T*>(*it->second.begin());
    }

    std::unordered_set<SceneComponent*> findAllByName(const std::string& name) const;
    std::unordered_set<SceneComponent*> findAllByTag(const std::string& tag) const;
    template <typename T>
    std::unordered_set<T*> findAllByType() const {
        static_assert(std::is_base_of<SceneComponent, T>::value, "T must derive from SceneComponent");
        auto it = m_typeIndex.find(typeid(T));
        if (it == m_typeIndex.end()) return {};

        std::unordered_set<T*> result;
        for (const SceneComponent* object : it->second) {
            result.insert(static_cast<T*>(object));
        }
        return result;
    }

    inline size_t objectCount() const { return m_allSceneObjects.size(); }

    inline const std::vector<Light*>& getLights() const { return m_lights; }

    inline const std::vector<Updatable*>& getUpdatables() const { return m_updatables; }

    inline const std::vector<Renderable*>& getRenderables() const { return m_renderables; }

    void update(float deltaTime) override;
};

#endif
