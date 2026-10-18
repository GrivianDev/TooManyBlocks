#ifndef TOOMANYBLOCKS_POLYMORPHICREGISTRY_H
#define TOOMANYBLOCKS_POLYMORPHICREGISTRY_H

#include <tuple>
#include <type_traits>

#include "engine/datastructures/PointerRegistry.h"

template <typename Base, typename... Types>
class PolymorphicRegistry {
private:
    std::tuple<PointerRegistry<Types>...> m_registries;

    template <typename T>
    PointerRegistry<T>& getRegistry() {
        return std::get<PointerRegistry<T>>(m_registries);
    }

    template <typename T>
    const PointerRegistry<T>& getRegistry() const {
        return std::get<PointerRegistry<T>>(m_registries);
    }

    template <typename T>
    void registerType(Base* object) {
        if (T* ptr = dynamic_cast<T*>(object)) {
            getRegistry<T>().add(ptr);
        }
    }

    template <typename T>
    void unregisterType(Base* object) {
        if (T* ptr = dynamic_cast<T*>(object)) {
            getRegistry<T>().remove(ptr);
        }
    }

public:
    static_assert(std::is_polymorphic_v<Base>, "Base must be a polymorphic type");

    void registerObject(Base* object) {
        if (!object) return;
        (registerType<Types>(object), ...);
    }

    void unregisterObject(Base* object) {
        if (!object) return;
        (unregisterType<Types>(object), ...);
    }

    template <typename T>
    const std::vector<T*>& get() const {
        static_assert((std::is_same_v<T, Types> || ...), "Requested type is not registered");
        return getRegistry<T>().getElements();
    }

    template <typename T>
    size_t size() const {
        static_assert((std::is_same_v<T, Types> || ...), "Requested type is not registered");
        return getRegistry<T>().size();
    }
};

#endif
