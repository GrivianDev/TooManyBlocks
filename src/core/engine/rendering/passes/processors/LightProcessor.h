#ifndef TOOMANYBLOCKS_LIGHTPROCESSOR_H
#define TOOMANYBLOCKS_LIGHTPROCESSOR_H

#include <array>
#include <vector>

#include "engine/scene/lights/Light.h"
#include "engine/rendering/opengl/FrameBuffer.h"
#include "engine/rendering/opengl/Texture.h"
#include "engine/rendering/opengl/UniformBuffer.h"
#include "engine/rendering/passes/allocator/ShadowMapAllocator.h"

struct RenderContext;

class LightProcessor {
private:
    size_t m_totalSupportedLights;
    ShadowMapAllocator m_shadowMapAllocator;
    std::array<FrameBuffer, SHADOW_ATLAS_COUNT> m_shadowAtlases;
    std::array<Texture*, SHADOW_ATLAS_COUNT> m_shadowMapsTextures;

    unsigned int m_shadowAtlasSize;

    std::vector<GPULight> m_lightBuffer;
    UniformBuffer m_lightUniformBuffer;

    std::vector<GPUShadowMap> m_shadowMapBuffer;
    UniformBuffer m_shadowMapUniformBuffer;

    unsigned int importanceToInitialResolution(float score) const;

    unsigned int directionalShadowResolution(const Light* light) const;

    float shadowQualityBias(unsigned int shadowCasterCount) const;

    float scoreLight(const Light* light, const RenderContext& content) const;

public:
    LightProcessor(size_t totalSupportedLights);

    void clearShadowMaps();

    void prepareShadowPass(const Light* light, const GPUShadowMap& shadowMap);

    void prepareShadowData(
        const std::vector<Light*>& lights,
        std::vector<Light*>& outputBuffer,
        const RenderContext& context
    );

    inline const std::array<Texture*, SHADOW_ATLAS_COUNT>& getShadowMapAtlases() const { return m_shadowMapsTextures; };

    inline unsigned int getShadowAtlasSize() const { return m_shadowAtlasSize; }

    inline const UniformBuffer* getLightUniformBuffer() const { return &m_lightUniformBuffer; }

    inline const std::vector<GPUShadowMap>& getAllocatedShadowMaps() const { return m_shadowMapBuffer; }

    inline const UniformBuffer* getShadowMapUniformBuffer() const { return &m_shadowMapUniformBuffer; }

    inline size_t getTotalSupportedLights() const { return m_totalSupportedLights; }
};

#endif
