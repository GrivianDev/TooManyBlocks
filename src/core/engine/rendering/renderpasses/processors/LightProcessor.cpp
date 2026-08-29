#include "LightProcessor.h"

#include <GL/glew.h>

#include <glm/glm.hpp>

#include "Logger.h"
#include "engine/env/lights/DirectionalLight.h"
#include "engine/env/lights/PointLight.h"
#include "engine/env/lights/Spotlight.h"
#include "engine/rendering/Frustum.h"
#include "engine/rendering/GLUtils.h"
#include "engine/rendering/Renderer.h"

constexpr unsigned int DESIRED_SHADOW_ATLAS_SIZE = 4096;
constexpr unsigned int MIN_SHADOWMAP_SIZE = 128;
constexpr unsigned int MAX_SHADOWMAP_COUNT = (DESIRED_SHADOW_ATLAS_SIZE / MIN_SHADOWMAP_SIZE) *
                                             (DESIRED_SHADOW_ATLAS_SIZE / MIN_SHADOWMAP_SIZE) * SHADOW_ATLAS_COUNT;

constexpr unsigned int POINTLIGHT_SHADOWMAP_COUNT = 6;
constexpr unsigned int FRUSTUM_CORNER_COUNT = 8;

struct ScoredLight {
    Light* lightPtr;
    float score;
    float importanceScore;
    unsigned int resolution;
    bool fixedResolution;
};

static void generateSpotShadowMapMatrix(std::vector<GPUShadowMap>& outMaps, const Spotlight* light) {
    Transform tr = light->getGlobalTransform();
    glm::vec3 pos = tr.getPosition();

    GPUShadowMap map{};
    map.viewProjection = glm::perspective(glm::radians(light->getFovy()), 1.0f, 0.1f, light->getRange()) *
                         glm::lookAt(pos, pos + tr.getForward(), tr.getUp());

    outMaps.push_back(map);
}

static void generateSpotLightShadowMapMatrix(std::vector<GPUShadowMap>& outMaps, const Spotlight* light) {
    Transform tr = light->getGlobalTransform();
    glm::vec3 pos = tr.getPosition();

    GPUShadowMap map{};
    map.viewProjection = glm::perspective(glm::radians(light->getFovy()), 1.0f, 0.1f, light->getRange()) *
                         glm::lookAt(pos, pos + tr.getForward(), tr.getUp());

    outMaps.push_back(map);
}

static void generatePointLightShadowMapMatrices(std::vector<GPUShadowMap>& outMaps, const PointLight* light) {
    constexpr glm::vec3 forward[POINTLIGHT_SHADOWMAP_COUNT] = {
        {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}
    };
    constexpr glm::vec3 up[POINTLIGHT_SHADOWMAP_COUNT] = {
        {0, -1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}, {0, -1, 0}, {0, -1, 0}
    };

    Transform tr = light->getGlobalTransform();
    glm::vec3 pos = tr.getPosition();

    glm::mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, light->getRange());

    for (unsigned int i = 0; i < POINTLIGHT_SHADOWMAP_COUNT; i++) {
        GPUShadowMap map{};
        map.viewProjection = projection * glm::lookAt(pos, pos + forward[i], up[i]);
        outMaps.push_back(map);
    }
}

static glm::mat4 getPerspectiveFar(glm::mat4 proj, float newFar) {
    // Assumes standard OpenGL perspective matrix
    float nearPlane = proj[3][2] / (proj[2][2] - 1.0f);

    float f = proj[2][2];

    proj[2][2] = -(newFar + nearPlane) / (newFar - nearPlane);
    proj[3][2] = -(2.0f * newFar * nearPlane) / (newFar - nearPlane);

    return proj;
};

static void generateDirectionalLightShadowMapMatrices(
    std::vector<GPUShadowMap>& outMaps,
    const DirectionalLight* light,
    const glm::mat4& cameraView,
    const glm::mat4& cameraProjection
) {
    constexpr float nearPlane = 0.1f;

    float splits[light->getCascadeCount() + 1];
    splits[0] = nearPlane;

    float lambda = light->getCascadeLambda();
    float shadowDistance = light->getShadowDistance();
    for (int i = 1; i <= light->getCascadeCount(); i++) {
        float fraction = static_cast<float>(i) / static_cast<float>(light->getCascadeCount());

        float log = nearPlane * std::pow(shadowDistance / nearPlane, fraction);
        float uniform = nearPlane + (shadowDistance - nearPlane) * fraction;
        splits[i] = lambda * log + (1.0f - lambda) * uniform;
    }

    Transform tr = light->getGlobalTransform();
    glm::mat4 shadowProjection = getPerspectiveFar(cameraProjection, shadowDistance);
    glm::mat4 invViewProj = glm::inverse(shadowProjection * cameraView);

    for (unsigned int cascade = 0; cascade < light->getCascadeCount(); cascade++) {
        constexpr glm::vec3 ndcCorners[FRUSTUM_CORNER_COUNT] = {
            {-1, -1, -1},
            {1, -1, -1},
            {1, 1, -1},
            {-1, 1, -1},

            {-1, -1, 1},
            {1, -1, 1},
            {1, 1, 1},
            {-1, 1, 1},
        };

        glm::vec3 fullFrustum[FRUSTUM_CORNER_COUNT];

        for (unsigned int i = 0; i < FRUSTUM_CORNER_COUNT; i++) {
            glm::vec4 p = invViewProj * glm::vec4(ndcCorners[i], 1);
            fullFrustum[i] = glm::vec3(p) / p.w;
        }

        float nearSplit = splits[cascade];
        float farSplit = splits[cascade + 1];

        glm::vec3 cascadeCorners[FRUSTUM_CORNER_COUNT];

        for (int i = 0; i < 4; i++) {
            glm::vec3 nearCorner = fullFrustum[i];
            glm::vec3 farCorner = fullFrustum[i + 4];

            glm::vec3 ray = farCorner - nearCorner;

            float nearFactor = (nearSplit - nearPlane) / (shadowDistance - nearPlane);
            float farFactor = (farSplit - nearPlane) / (shadowDistance - nearPlane);
            cascadeCorners[i] = nearCorner + ray * nearFactor;
            cascadeCorners[i + 4] = nearCorner + ray * farFactor;
        }

        glm::vec3 center(0.0f);
        for (const glm::vec3& c : cascadeCorners) center += c;
        center /= static_cast<float>(FRUSTUM_CORNER_COUNT);

        // Snap the cascade center to whole world units.
        // This reduces tiny camera movements causing shimmering shadows.
        center.x = std::round(center.x);
        center.y = std::round(center.y);
        center.z = std::round(center.z);

        glm::mat4 lightView = glm::lookAt(center - (tr.getForward() * 100.0f), center, tr.getUp());

        // Get "radius", largest absolute X or Y offset from the center for orthographic projection.
        float radius = 0.0f;
        glm::vec3 centerLS = glm::vec3(lightView * glm::vec4(center, 1.0f));
        for (const glm::vec3& corner : cascadeCorners) {
            glm::vec3 ls = glm::vec3(lightView * glm::vec4(corner, 1.0f));

            radius = std::max(radius, std::max(std::abs(ls.x - centerLS.x), std::abs(ls.y - centerLS.y)));
        }
        radius = ceil(radius / 4.0f) * 4.0f;  // snap value to 4 increments

        float minX = centerLS.x - radius;
        float maxX = centerLS.x + radius;

        float minY = centerLS.y - radius;
        float maxY = centerLS.y + radius;

        // Depth still comes from the actual frustum
        float minZ = FLT_MAX;
        float maxZ = -FLT_MAX;

        for (const glm::vec3& corner : cascadeCorners) {
            glm::vec3 ls = glm::vec3(lightView * glm::vec4(corner, 1));
            minZ = std::min(minZ, ls.z);
            maxZ = std::max(maxZ, ls.z);
        }

        // Extend the shadow volume backwards, along the light direction,
        // so objects outside the camera cascade can still cast shadows.
        maxZ += light->getShadowCasterDistance();

        glm::mat4 proj = glm::ortho(minX, maxX, minY, maxY, -maxZ, -minZ);

        GPUShadowMap map{};
        map.viewProjection = proj * lightView;
        map.cascadeSplit = splits[cascade + 1];
        outMaps.push_back(map);
    }
}

static void generateViewProjectionMatrices(
    std::vector<GPUShadowMap>& outMaps,
    const Light* light,
    const glm::mat4& cameraView,
    const glm::mat4& cameraProjection
) {
    outMaps.clear();

    switch (light->getType()) {
        case LightType::Spot: {
            generateSpotLightShadowMapMatrix(outMaps, static_cast<const Spotlight*>(light));
            break;
        }
        case LightType::Point: {
            generatePointLightShadowMapMatrices(outMaps, static_cast<const PointLight*>(light));
            break;
        }
        case LightType::Directional: {
            generateDirectionalLightShadowMapMatrices(
                outMaps, static_cast<const DirectionalLight*>(light), cameraView, cameraProjection
            );
            break;
        }
        default: throw std::runtime_error("Unhandled light type");
    }
}

static std::shared_ptr<Texture> makeShadowAtlas(unsigned int size) {
    std::shared_ptr<Texture> atlas = std::make_shared<Texture>(
        Texture::create(TextureType::Depth, size, size, 1, nullptr, TextureFilter::Linear, TextureWrap::ClampToEdge)
    );
    atlas->bind();
    GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE));
    GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL));

    return atlas;
}

unsigned int LightProcessor::importanceToInitialResolution(float importance) const {
    if (importance < 0.01f) return 0;
    if (importance < 0.15f) return MIN_SHADOWMAP_SIZE;
    if (importance < 0.35f) return 256;
    if (importance < 0.65f) return 512;
    if (importance < 0.9f) return 1024;
    return 2048;
}

unsigned int LightProcessor::directionalShadowResolution(const Light* light) const {
    const DirectionalLight* dirLight = static_cast<const DirectionalLight*>(light);
    unsigned int maxResolution = m_shadowMapAllocator.getMaxUniformResolutionInAtlas(dirLight->getCascadeCount());

    float distanceFactor = dirLight->getShadowDistance() / 100.0f;

    unsigned int desired = distanceFactor > 1.0f ? 2048 : 1024;
    return std::min(desired, maxResolution);
}

float LightProcessor::scoreLight(const Light* light, const RenderContext& context) const {
    if (light->getIntensity() < 0.001f) return 0.0f;

    glm::vec3 cameraPos = context.tInfo.viewportTransform.getPosition();
    glm::vec3 lightPos = light->getGlobalTransform().getPosition();

    float distance = glm::distance(cameraPos, lightPos);
    float angularRadius = std::atan(light->getRange() / std::max(distance, 0.01f));

    float screenFactor = std::clamp(std::pow(angularRadius / glm::radians(45.0f), 2.0f), 0.0f, 1.0f);

    float priorityFactor = std::max(light->getShadowPriority(), 0.0f);

    float visibilityFactor = light->getIntensity() > 0.001f ? 1.0f : 0.0f;

    return screenFactor * priorityFactor * visibilityFactor;
}

LightProcessor::LightProcessor(size_t totalSupportedLights) : m_totalSupportedLights(totalSupportedLights) {
    int maxTextureSize;
    GLCALL(glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize));
    m_shadowAtlasSize = std::min<unsigned int>(maxTextureSize, DESIRED_SHADOW_ATLAS_SIZE);

    m_shadowMapAllocator = ShadowMapAllocator(SHADOW_ATLAS_COUNT, m_shadowAtlasSize, MIN_SHADOWMAP_SIZE);

    // Create buffers for shadowmapping
    for (unsigned int i = 0; i < m_shadowAtlases.size(); i++) {
        FrameBuffer fb = FrameBuffer::create();
        fb.attachTexture(makeShadowAtlas(m_shadowAtlasSize));
        m_shadowMapsTextures[i] = fb.getAttachedDepthTexture().get();
        m_shadowAtlases[i] = std::move(fb);
    }

    m_lightBuffer.reserve(m_totalSupportedLights);
    m_lightUniformBuffer = UniformBuffer::create(nullptr, m_totalSupportedLights * sizeof(GPULight));

    m_shadowMapBuffer.reserve(MAX_SHADOWMAP_COUNT);
    m_shadowMapUniformBuffer = UniformBuffer::create(nullptr, MAX_SHADOWMAP_COUNT * sizeof(GPUShadowMap));
}

void LightProcessor::clearShadowMaps() {
    for (const FrameBuffer& shadowAtlasFrameBuffer : m_shadowAtlases) {
        shadowAtlasFrameBuffer.bind();
        GLCALL(glClear(GL_DEPTH_BUFFER_BIT));
    }
}

void LightProcessor::prepareShadowPass(const Light* light, const GPUShadowMap& shadowMap) {
    int x = shadowMap.atlasOffset.x * m_shadowAtlasSize;
    int y = shadowMap.atlasOffset.y * m_shadowAtlasSize;
    int shadowMapWidth = shadowMap.atlasScale.x * m_shadowAtlasSize;
    int shadowMapHeight = shadowMap.atlasScale.y * m_shadowAtlasSize;

    m_shadowAtlases[shadowMap.atlasIndex].bind();
    GLCALL(glViewport(x, y, shadowMapWidth, shadowMapHeight));
}

void LightProcessor::prepareShadowData(
    const std::vector<Light*>& lights,
    std::vector<Light*>& outputBuffer,
    const RenderContext& context
) {
    const Frustum cameraFrustum(context.tInfo.viewProjection);

    m_shadowMapAllocator.clear();
    m_shadowMapBuffer.clear();
    outputBuffer.clear();

    std::vector<ScoredLight> scoredLights;
    DirectionalLight* directionalLight = nullptr;
    unsigned int directionalLightCascadeResolution = 0;
    for (Light* light : lights) {
        light->setShadowMapCount(0);  // Reset shadow map count

        if (light->getType() == LightType::Directional) {
            if (!directionalLight && light->castsShadows()) directionalLight = static_cast<DirectionalLight*>(light);
            continue;
        } else if (!cameraFrustum.isSphereInside(light->getGlobalTransform().getPosition(), light->getRange())) {
            continue;
        }

        if (!light->castsShadows()) {
            // Forward non shadow casting lights without shadow priorization
            outputBuffer.push_back(light);
            continue;
        };

        float score = scoreLight(light, context);
        scoredLights.push_back({light, score, 0.0f, 0, false});
    }

    std::sort(scoredLights.begin(), scoredLights.end(), [](const ScoredLight& a, const ScoredLight& b) {
        return a.score > b.score;
    });

    uint64_t totalCost = 0;
    uint64_t budget = m_shadowMapAllocator.atlasCount() * m_shadowMapAllocator.atlasSize() *
                      m_shadowMapAllocator.atlasSize();

    if (directionalLight) {
        directionalLightCascadeResolution = directionalShadowResolution(directionalLight);
        uint64_t directionalCost = static_cast<uint64_t>(directionalLightCascadeResolution) *
                                   directionalLightCascadeResolution * directionalLight->getCascadeCount();
        totalCost += directionalCost;
    }

    const float maxScore = scoredLights.empty() ? 1.0f : scoredLights.front().score;
    for (ScoredLight& scored : scoredLights) {
        // Derive importance from absolute scene relevance and relative scored to each other
        constexpr float fullAbssluteRelevanceThreshold = 0.3f;
        float absoluteScore = std::clamp(scored.score / fullAbssluteRelevanceThreshold, 0.0f, 1.0f);
        float relativeScore = scored.score / maxScore;
        scored.importanceScore = 0.7f * absoluteScore + 0.3f * relativeScore;
        scored.resolution = importanceToInitialResolution(scored.importanceScore);
        totalCost += scored.resolution * scored.resolution * scored.lightPtr->faceCount();
    }

    while (totalCost > budget) {
        // Reduce resolution of shadow maps that are not as important but have large desired resolution
        int bestIndex = -1;
        float leastPenalty = FLT_MAX;

        for (size_t i = 0; i < scoredLights.size(); i++) {
            ScoredLight& light = scoredLights[i];

            if (light.resolution < MIN_SHADOWMAP_SIZE || light.fixedResolution) continue;

            unsigned int nextResolution = (light.resolution == MIN_SHADOWMAP_SIZE) ? 0 : light.resolution / 2;

            uint64_t currentCost = static_cast<uint64_t>(light.resolution) * light.resolution *
                                   light.lightPtr->faceCount();
            uint64_t nextCost = static_cast<uint64_t>(nextResolution) * nextResolution * light.lightPtr->faceCount();
            uint64_t savedCost = currentCost - nextCost;

            if (savedCost == 0) continue;

            // How much visual importance would be lost
            float qualityLoss = light.importanceScore * (1.0f - float(nextResolution) / light.resolution);

            // Lower means: cheap quality loss for lots of saved space
            float penalty = qualityLoss / float(savedCost);

            if (penalty < leastPenalty) {
                leastPenalty = penalty;
                bestIndex = static_cast<int>(i);
            }
        }

        if (bestIndex == -1) break;

        ScoredLight& light = scoredLights[bestIndex];

        uint64_t currentCost = static_cast<uint64_t>(light.resolution) * light.resolution * light.lightPtr->faceCount();
        light.resolution /= 2;
        uint64_t nextCost = static_cast<uint64_t>(light.resolution) * light.resolution * light.lightPtr->faceCount();

        totalCost -= currentCost - nextCost;
    }

    if (directionalLight) {  // Dodge scoring / adjusting logic for directional lights
        scoredLights.insert(
            scoredLights.begin(), {directionalLight, 1.0f, 1.0f, directionalLightCascadeResolution, true}
        );
    }

    for (const ScoredLight& scored : scoredLights) {
        if (scored.resolution != 0) {
            std::vector<ShadowMapAllocator::Allocation> allocations;
            std::vector<GPUShadowMap> generatedMaps;

            generateViewProjectionMatrices(
                generatedMaps, scored.lightPtr, context.tInfo.view, context.tInfo.projection
            );

            if (m_shadowMapAllocator.allocate(scored.resolution, generatedMaps.size(), allocations)) {
                scored.lightPtr->setShadowMapsOffset(static_cast<unsigned int>(m_shadowMapBuffer.size()));
                scored.lightPtr->setShadowMapCount(static_cast<unsigned int>(generatedMaps.size()));

                for (size_t i = 0; i < generatedMaps.size(); i++) {
                    generatedMaps[i].lightIndex = outputBuffer.size();

                    generatedMaps[i].atlasIndex = allocations[i].atlasIndex;
                    generatedMaps[i].resolution = allocations[i].resolution;
                    generatedMaps[i].atlasOffset = allocations[i].atlasOffset;
                    generatedMaps[i].atlasScale = allocations[i].atlasScale;

                    m_shadowMapBuffer.push_back(generatedMaps[i]);
                }
            } else {
                lgr::lout.error("FAILED shadow map allocation for resolution " + std::to_string(scored.resolution));
            }
        }

        // Push lights that have participated in shadow map priorization prozess
        outputBuffer.push_back(scored.lightPtr);
    }

    // Upload lights and shadow data
    m_lightBuffer.clear();
    for (Light* light : outputBuffer) {
        m_lightBuffer.push_back(light->toGPULight());
    }
    m_lightUniformBuffer.updateData(m_lightBuffer.data(), m_lightBuffer.size() * sizeof(GPULight));
    m_shadowMapUniformBuffer.updateData(m_shadowMapBuffer.data(), m_shadowMapBuffer.size() * sizeof(GPUShadowMap));
}
