#include "AssetManager.h"

void AssetManager::update(float deltaTime) {
    std::lock_guard<std::mutex> lock(m_mtx);
    
    for (auto it = m_representations.begin(); it != m_representations.end();) {
        IAssetRepresentation* rep = it->second.get();

        switch (rep->cache.policy) {
            case CachePolicy::Persist: {
                ++it;
                break;
            }

            case CachePolicy::None: {
                m_assetPolicyOverrides.erase(it->first);
                it = m_representations.erase(it);
                break;
            }

            case CachePolicy::RefCounted: {
                if (rep->useCount() == 1) {
                    m_assetPolicyOverrides.erase(it->first);
                    it = m_representations.erase(it);
                } else {
                    ++it;
                }
                break;
            }

            case CachePolicy::GracePeriod: {
                if (rep->useCount() == 1) {
                    rep->accumulatedTtl += deltaTime;

                    if (rep->accumulatedTtl >= rep->cache.ttl) {
                        m_assetPolicyOverrides.erase(it->first);
                        it = m_representations.erase(it);
                        break;
                    }
                } else {
                    rep->accumulatedTtl = 0.0f;
                }

                ++it;
                break;
            }
        }
    }
}