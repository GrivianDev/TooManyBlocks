#define MAX_LIGHTS 1024
#define MAX_SHADOWMAP_COUNT 4096
#define SHADOWATLAS_COUNT 4

#define DIRECTIONALLIGHT 0u
#define SPOTLIGHT 1u
#define POINTLIGHT 2u

struct Light {
    uint lightType;
    uint shadowMapsOffset;
    uint shadowMapCount;
    float intensity;
    vec3 lightPosition;
    float range; // Used by point- / spotlight
    vec3 direction;
    float fovy; // Used by spotlicht
    vec3 color;
    float innerCutoffAngle; // Used by spotlicht
};

struct ShadowMap {
    mat4 viewProjection;

    uint lightIndex;
    uint atlasIndex;
    uint resolution;
    float cascadeSplit;

    vec2 atlasOffset;  // [0, 1] Normalized to atlas size
    vec2 atlasScale;   // [0, 1] Normalized to atlas size
};

layout(std140) uniform LightsBlock {
    Light u_lights[MAX_LIGHTS];
};

layout(std140) uniform ShadowMapsBlock {
    ShadowMap u_shadowMaps[MAX_SHADOWMAP_COUNT];
};

uniform int u_lightCount;

uniform sampler2DShadow u_shadowMapAtlas[SHADOWATLAS_COUNT];

vec3 _calcLightContribution(int lightIndex, mat4 cameraViewMatrix, vec3 cameraPosition, vec3 texelPosition, vec3 texelNormal) {
    uint lightType = u_lights[lightIndex].lightType;
    uint shadowMapsOffset = u_lights[lightIndex].shadowMapsOffset;
    uint shadowMapCount = u_lights[lightIndex].shadowMapCount;

    vec3 lightPosition = u_lights[lightIndex].lightPosition;
    vec3 direction = u_lights[lightIndex].direction;
    vec3 color = u_lights[lightIndex].color;

    float intensity = u_lights[lightIndex].intensity;
    float range = u_lights[lightIndex].range;
    float fovy = u_lights[lightIndex].fovy;
    float innerCutoffAngle = u_lights[lightIndex].innerCutoffAngle;

    vec3 L = lightPosition - texelPosition;
    float dist2 = dot(L, L);

    if(lightType != DIRECTIONALLIGHT && dist2 >= range * range) {
        return vec3(0.0);
    }

    // Lighting direction
    vec3 lightDirection;
    float distanceToLight = 0.0;
    if(lightType == DIRECTIONALLIGHT) {
        lightDirection = -direction;
    } else {
        distanceToLight = sqrt(dist2);
        lightDirection = L / distanceToLight;
    }

    // Diffuse
    float diffuseFactor = max(dot(texelNormal, lightDirection), 0.0);
    if(diffuseFactor <= 0.0)
        return vec3(0.0);

    // Specular
    const float specularExponent = 30.0;
    vec3 viewDirection = normalize(cameraPosition - texelPosition);
    vec3 reflectionDirection = reflect(-lightDirection, texelNormal);
    float specularFactor = pow(max(dot(viewDirection, reflectionDirection), 0.0), specularExponent);

    // Spotlight falloff
    float spotFactor = 1.0;
    if(lightType == SPOTLIGHT) {
        float angle = degrees(acos(dot(-direction, lightDirection)));
        spotFactor = 1.0 -
            smoothstep(innerCutoffAngle * 0.5, fovy * 0.5, angle);
    }

    // Distance falloff
    float falloff = 1.0;
    if(lightType != DIRECTIONALLIGHT) {
        falloff = 1.0 -
            smoothstep(range * 0.9, range, distanceToLight);
    }

    float lightFactor = 1.0;
    if(shadowMapCount > 0u) {
        // Shadow map lookup
        if(lightType == DIRECTIONALLIGHT) {

            // Select the cascade based on the fragment's distance from the camera.
            float viewDepth = -(cameraViewMatrix * vec4(texelPosition, 1.0)).z;

            ShadowMap shadowMap = u_shadowMaps[shadowMapsOffset];

            bool foundCascade = false;
            for(uint i = 0u; i < shadowMapCount; ++i) {
                ShadowMap candidate = u_shadowMaps[shadowMapsOffset + i];

                if(viewDepth <= candidate.cascadeSplit) {
                    shadowMap = candidate;
                    foundCascade = true;
                    break;
                }
            }

            if(foundCascade) {
                vec4 lightSpacePosition = shadowMap.viewProjection * vec4(texelPosition, 1.0);

                vec3 lightSpaceCoord = lightSpacePosition.xyz / lightSpacePosition.w;
                lightSpaceCoord = lightSpaceCoord * 0.5 + 0.5;

                // Outside cascade projection -> no shadow information
                if(lightSpaceCoord.x >= 0.0 && lightSpaceCoord.x <= 1.0 &&
                    lightSpaceCoord.y >= 0.0 && lightSpaceCoord.y <= 1.0 &&
                    lightSpaceCoord.z >= 0.0 && lightSpaceCoord.z <= 1.0) {

                    vec2 atlasUV = shadowMap.atlasOffset +
                        lightSpaceCoord.xy * shadowMap.atlasScale;
                    lightFactor = texture(u_shadowMapAtlas[shadowMap.atlasIndex], vec3(atlasUV, lightSpaceCoord.z));
                }
            }
        } else {
            // Spotlights and point lights: find the shadow map that contains
            // the fragment. Spotlights have one map, point lights have six.
            for(uint i = 0u; i < shadowMapCount; ++i) {

                ShadowMap shadowMap = u_shadowMaps[shadowMapsOffset + i];

                vec4 lightSpacePosition = shadowMap.viewProjection * vec4(texelPosition, 1.0);

                vec3 lightSpaceCoord = lightSpacePosition.xyz / lightSpacePosition.w;
                lightSpaceCoord = lightSpaceCoord * 0.5 + 0.5;

                if(lightSpaceCoord.x < 0.0 || lightSpaceCoord.x > 1.0 ||
                    lightSpaceCoord.y < 0.0 || lightSpaceCoord.y > 1.0 ||
                    lightSpaceCoord.z < 0.0 || lightSpaceCoord.z > 1.0) {
                    continue;
                }

                vec2 atlasUV = shadowMap.atlasOffset +
                    lightSpaceCoord.xy * shadowMap.atlasScale;

                lightFactor = texture(u_shadowMapAtlas[shadowMap.atlasIndex], vec3(atlasUV, lightSpaceCoord.z));
                break;
            }
        }
    }

    return color *
        intensity *
        falloff *
        spotFactor *
        lightFactor *
        (diffuseFactor + specularFactor);
}

vec3 accumulateLightContributions(mat4 cameraViewMatrix, vec3 cameraPosition, vec3 texelPosition, vec3 texelNormal) {
    vec3 lightContrib = vec3(0.0);
    for(int i = 0; i < u_lightCount; i++) {
        lightContrib += _calcLightContribution(i, cameraViewMatrix, cameraPosition, texelPosition, texelNormal);
    }
    return lightContrib;
}