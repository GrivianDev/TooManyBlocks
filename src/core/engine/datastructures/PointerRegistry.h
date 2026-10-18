#ifndef TOOMANYBLOCKS_POINTERPEREGISTRY_H
#define TOOMANYBLOCKS_POINTERPEREGISTRY_H

#include <algorithm>
#include <vector>

template <typename T>
class PointerRegistry {
private:
    std::vector<T*> m_components;

public:
    void add(T* component) {
        if (!component) return;
        m_components.push_back(component);
    }

    void remove(T* component) {
        auto it = std::find(m_components.begin(), m_components.end(), component);
        if (it != m_components.end()) {
            *it = m_components.back();
            m_components.pop_back();
        }
    }

    const std::vector<T*>& getElements() const { return m_components; }

    size_t size() const { return m_components.size(); }
};

#endif
