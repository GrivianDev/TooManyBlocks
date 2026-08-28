#ifndef TOOMANYBLOCKS_ASSETMANAGER_H
#define TOOMANYBLOCKS_ASSETMANAGER_H

#include <cinttypes>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <typeindex>
#include <unordered_map>

#include "Logger.h"
#include "engine/Updatable.h"
#include "foundation/threading/Future.h"

class AssetManager;

struct AssetHandle {
    uint64_t id;

    constexpr bool operator==(const AssetHandle& a) const { return id == a.id; }
};

template <>
struct std::hash<AssetHandle> {
    size_t operator()(const AssetHandle& h) const { return std::hash<uint64_t>{}(h.id); }
};

struct AssetKey {
    AssetHandle handle;
    std::type_index type;

    bool operator==(const AssetKey& a) const { return handle == a.handle && type == a.type; };
};

struct AssetKeyHash {
    size_t operator()(const AssetKey& key) const {
        return std::hash<uint64_t>{}(key.handle.id) ^ std::hash<std::type_index>{}(key.type);
    }
};

struct IAssetSourceHolder {
public:
    virtual ~IAssetSourceHolder() = default;
};

template <typename T>
struct AssetSourceHolder : public IAssetSourceHolder {
    T source;

    explicit AssetSourceHolder(const T& sourceValue) : source(sourceValue) {}
};

struct AssetRecord {
    std::unique_ptr<IAssetSourceHolder> source;
};

enum class CachePolicy {
    Persist,
    GracePeriod,
    RefCounted,
    None
};

struct CacheMetadata {
    CachePolicy policy;
    float ttl;
};

struct IAssetRepresentation {
    CacheMetadata cache;
    float accumulatedTtl;

    virtual ~IAssetRepresentation() = default;
    virtual size_t useCount() const = 0;
};

template <typename T>
struct AssetRepresentation : IAssetRepresentation {
    Future<T> future;

    AssetRepresentation(const Future<T>& future, CacheMetadata cache) : future(future) { this->cache = cache; }
    virtual ~AssetRepresentation() {
        if (future.hasError()) {
            lgr::lout.error("Destroying failed asset representation: " + std::string(typeid(T).name()));
        }
    }

    virtual size_t useCount() const override { return future.useCount(); }
};

struct IRepresentationFactory {
    virtual ~IRepresentationFactory() = default;
};

template <typename T>
struct RepresentationFactory : IRepresentationFactory {
    std::function<Future<T>(AssetHandle, AssetManager&)> create;

    explicit RepresentationFactory(std::function<Future<T>(AssetHandle, AssetManager&)>&& createFn)
        : create(std::move(createFn)) {}
};

class AssetManager : public Updatable {
private:
    uint64_t m_nextId;

    mutable std::mutex m_mtx;

    std::unordered_map<AssetHandle, AssetRecord> m_assets;
    std::unordered_map<AssetKey, std::shared_ptr<IAssetRepresentation>, AssetKeyHash> m_representations;
    std::unordered_map<std::type_index, std::unique_ptr<IRepresentationFactory>> m_factories;

    CacheMetadata m_defaultPolicy;
    std::unordered_map<std::type_index, CacheMetadata> m_typePolicyOverrides;
    std::unordered_map<AssetKey, CacheMetadata, AssetKeyHash> m_assetPolicyOverrides;

    template <typename T>
    const Future<T>& retrieve(const std::shared_ptr<IAssetRepresentation>& representation) {
        auto* typed = dynamic_cast<AssetRepresentation<T>*>(representation.get());

        if (!typed) throw std::runtime_error("Representation type mismatch");

        return typed->future;
    }

    template <typename T>
    const RepresentationFactory<T>& getFactory() const {
        auto it = m_factories.find(typeid(T));
        if (it == m_factories.end()) {
            throw std::runtime_error("No representation provider registered");
        }

        auto* provider = dynamic_cast<RepresentationFactory<T>*>(it->second.get());
        if (!provider) {
            throw std::runtime_error("Representation provider type mismatch");
        }

        return *provider;
    }

    inline AssetHandle create() { return AssetHandle{m_nextId++}; }

    template <typename Source>
    void attachSource(AssetHandle handle, const Source& source) {
        m_assets.emplace(handle, AssetRecord{std::make_unique<AssetSourceHolder<Source>>(source)});
    };

    inline CacheMetadata resolveCachePolicy(AssetKey key) const {
        auto overridePolicy = m_assetPolicyOverrides.find(key);
        if (overridePolicy != m_assetPolicyOverrides.end()) return overridePolicy->second;

        auto def = m_typePolicyOverrides.find(key.type);
        if (def != m_typePolicyOverrides.end()) return def->second;

        return m_defaultPolicy;
    }

    template <typename T>
    Future<T> provide(AssetHandle handle, Future<T> future) {
        AssetKey key{handle, typeid(T)};

        auto [it, inserted] = m_representations.emplace(
            key, std::make_shared<AssetRepresentation<T>>(future, resolveCachePolicy(key))
        );

        if (!inserted) {
            throw std::runtime_error("Representation already exists");
        }

        return future;
    }

public:
    inline AssetManager() noexcept : m_nextId(1), m_defaultPolicy({CachePolicy::None, 0.0f}) {}

    template <typename Source>
    const Source& getSource(AssetHandle handle) const {
        std::lock_guard<std::mutex> lock(m_mtx);
        auto* holder = dynamic_cast<AssetSourceHolder<Source>*>(m_assets.at(handle).source.get());
        if (!holder) throw std::runtime_error("Invalid source type");

        return holder->source;
    }

     // Registered factories should construct an already started future
    template <typename T>
    void registerFactory(std::function<Future<T>(AssetHandle, AssetManager&)> factory) {
        std::lock_guard<std::mutex> lock(m_mtx);
        m_factories[typeid(T)] = std::make_unique<RepresentationFactory<T>>(std::move(factory));
    }

    template <typename T>
    void setCachePolicy(CachePolicy policy, float ttl = 0.0f) {
        std::lock_guard<std::mutex> lock(m_mtx);
        m_typePolicyOverrides[typeid(T)] = {policy, ttl};
    }

    template <typename T>
    void setCachePolicy(AssetHandle handle, CachePolicy policy, float ttl = 0.0f) {
        std::lock_guard<std::mutex> lock(m_mtx);
        m_assetPolicyOverrides[{handle, typeid(T)}] = {policy, ttl};
    }

    void setDefaultCachePolicy(CachePolicy policy, float ttl = 0.0f) {
        std::lock_guard<std::mutex> lock(m_mtx);
        m_defaultPolicy = {policy, ttl};
    }

    template <typename Source>
    AssetHandle import(const Source& source) {
        std::lock_guard<std::mutex> lock(m_mtx);

        AssetHandle handle = create();
        attachSource(handle, source);
        return handle;
    }

    template <typename T>
    Future<T> request(AssetHandle handle) {
        AssetKey key{handle, typeid(T)};
        Future<T> result;

        {
            std::lock_guard<std::mutex> lock(m_mtx);
            auto it = m_representations.find(key);
            if (it != m_representations.end()) {
                return retrieve<T>(it->second);
            }
            result = provide<T>(handle, Future<T>::deferred());
        }

        result.resolve(getFactory<T>().create(handle, *this));
        return result;
    }

    void update(float deltaTime) override;
};

#endif
