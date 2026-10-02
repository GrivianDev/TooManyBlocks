#include "Scene.h"

#include <algorithm>

constexpr float SPATIAL_FAT_AABB_MARGIN = 0.5f;

void Scene::onNameChanged(SceneComponent* component, const std::string& oldName, const std::string& newName) {
    if (!oldName.empty()) {
        auto it = m_nameIndex.find(oldName);
        if (it != m_nameIndex.end()) {
            it->second.erase(component);

            if (it->second.empty()) {
                m_nameIndex.erase(it);
            }
        }
    }

    if (!newName.empty()) {
        m_nameIndex[newName].insert(component);
    }
}

void Scene::onTagAdded(SceneComponent* component, const std::string& tag) { m_tagIndex[tag].insert(component); }

void Scene::onTagRemoved(SceneComponent* component, const std::string& tag) {
    auto it = m_tagIndex.find(tag);
    if (it == m_tagIndex.end()) return;

    it->second.erase(component);

    if (it->second.empty()) {
        m_tagIndex.erase(it);
    }
}

void Scene::onSpatialDirty(SceneComponent* component) { m_spatialDirty.push_back(component); }

Scene::Scene() : m_spatialAabbTree(SPATIAL_FAT_AABB_MARGIN) {}

Scene::~Scene() {
    while (!m_allSceneObjects.empty()) {
        SceneComponent* component = m_allSceneObjects.begin()->first;
        destroy(component);
    }
}

void Scene::destroy(SceneComponent* component) {
    if (!component) return;
    if (component->scene != this) return;
    component->scene = nullptr;

    const std::vector<SceneComponent*> children = component->children;  // copy
    for (SceneComponent* child : children) {
        destroy(child);
    }

    // Remove from name index
    {
        auto it = m_nameIndex.find(component->getName());
        if (it != m_nameIndex.end()) {
            it->second.erase(component);

            if (it->second.empty()) {
                m_nameIndex.erase(it);
            }
        }
    }

    // Remove from tag index
    for (const std::string& tag : component->getTags()) {
        auto it = m_tagIndex.find(tag);
        if (it != m_tagIndex.end()) {
            it->second.erase(component);

            if (it->second.empty()) {
                m_tagIndex.erase(it);
            }
        }
    }

    // Remove from type index
    {
        std::type_index type = typeid(*component);

        auto it = m_typeIndex.find(type);
        if (it != m_typeIndex.end()) {
            it->second.erase(component);

            if (it->second.empty()) {
                m_typeIndex.erase(it);
            }
        }
    }

    // Unregister lights
    {
        Light* light = dynamic_cast<Light*>(component);
        if (light) {
            auto it = std::find(m_lights.begin(), m_lights.end(), light);
            if (it != m_lights.end()) {
                *it = m_lights.back();
                m_lights.pop_back();
            }
        };
    }

    // Unregister updatable
    {
        Updatable* updatable = dynamic_cast<Updatable*>(component);
        if (updatable) {
            auto it = std::find(m_updatables.begin(), m_updatables.end(), updatable);
            if (it != m_updatables.end()) {
                *it = m_updatables.back();
                m_updatables.pop_back();
            }
        }
    }

    // Unregister renderable
    {
        Renderable* renderable = dynamic_cast<Renderable*>(component);
        if (renderable) {
            auto it = std::find(m_renderables.begin(), m_renderables.end(), renderable);
            if (it != m_renderables.end()) {
                *it = m_renderables.back();
                m_renderables.pop_back();
            }
        }
    }

    // Remove from aabb tree
    if (component->getSpatialIndex() >= 0) {
        m_spatialAabbTree.remove(component->getSpatialIndex());
    }

    m_spatialDirty.erase(std::remove(m_spatialDirty.begin(), m_spatialDirty.end(), component), m_spatialDirty.end());

    m_allSceneObjects.erase(component);
}

SceneComponent* Scene::findByName(const std::string& name) const {
    auto it = m_nameIndex.find(name);
    if (it == m_nameIndex.end() || it->second.empty()) return nullptr;
    return *it->second.begin();
}

SceneComponent* Scene::findByTag(const std::string& tag) const {
    auto it = m_tagIndex.find(tag);
    if (it == m_tagIndex.end() || it->second.empty()) return nullptr;
    return *it->second.begin();
}

std::unordered_set<SceneComponent*> Scene::findAllByName(const std::string& name) const {
    auto it = m_nameIndex.find(name);
    if (it == m_nameIndex.end()) return {};
    return it->second;
}

std::unordered_set<SceneComponent*> Scene::findAllByTag(const std::string& tag) const {
    auto it = m_tagIndex.find(tag);
    if (it == m_tagIndex.end()) return {};
    return it->second;
}

void Scene::query(const BoundingBox& bounds, std::function<void(Renderable*)> callback) const {
    m_spatialAabbTree.query(bounds, [&callback](SceneComponent* object) {
        if (Renderable* r = dynamic_cast<Renderable*>(object)) {
            callback(r);
        }
    });
}

void Scene::update(float deltaTime) {
    for (Updatable* u : m_updatables) {
        u->update(deltaTime);
    }
    for (SceneComponent* c : m_spatialDirty) {
        const BoundingBox& bounds = c->getGlobalBounds();
        if (bounds.isInvalid()) {
            if (c->getSpatialIndex() >= 0) {
                m_spatialAabbTree.remove(c->getSpatialIndex());
                c->setSpatialIndex(INVALID_TREE_NODE);
            }
            continue;
        }

        if (c->getSpatialIndex() >= 0) {
            m_spatialAabbTree.update(c->getSpatialIndex(), c->getGlobalBounds());
        } else {
            int spatialIndex = m_spatialAabbTree.insert(c, c->getGlobalBounds());
            c->setSpatialIndex(spatialIndex);
        }
    }
    m_spatialDirty.clear();
}
