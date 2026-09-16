#version 430 core

#define SSAO_SAMPLE_COUNT 32u

layout(location = 0) out float ssaoValue;

in vec2 screenUV;

uniform sampler2D u_positionTexture;
uniform sampler2D u_normalTexture;
uniform sampler2D u_noiseTexture;
uniform float u_noiseTextureScale;
uniform vec3 u_kernelSamples[SSAO_SAMPLE_COUNT];

uniform mat4 u_projection;

const float radius = 0.8;
const float bias = 0.1;

void main() {
    // Get position and normal (Both are in view space)
    vec3 fragPos = texture(u_positionTexture, screenUV).xyz;
    vec3 normal = texture(u_normalTexture, screenUV).xyz;

    // !Assumes u_positionTexture has the same resolution as the target ssao texture!
    vec2 noiseScale = vec2(textureSize(u_positionTexture, 0)) / u_noiseTextureScale;
    vec3 randomNoiseVec = vec3(texture(u_noiseTexture, screenUV * noiseScale).xy, 0.0);

    // Create TBN matrix: Converts tagent space to view space
    vec3 tagent = normalize(randomNoiseVec - normal * dot(randomNoiseVec, normal) + 0.0001);
    vec3 biTangent = cross(normal, tagent);
    mat3 TBN = mat3(tagent, biTangent, normal);

    float occlusion = 0.0;
    for (uint i = 0; i < SSAO_SAMPLE_COUNT; i++) {
        // Transform sample into viewspace
        vec3 samplePos = TBN * u_kernelSamples[i];
        samplePos = fragPos + samplePos * radius;

        vec4 offset = u_projection * vec4(samplePos, 1.0);
        vec2 texCoord = offset.xy / offset.w * 0.5 + 0.5; // Sample coord in range [0, 1]
        vec3 occluderPos = texture(u_positionTexture, texCoord).xyz;

        // Range check to prevent distant occluders take effect in view space
        float rangeCheck = smoothstep(0.0, 1.0, radius / length(fragPos - occluderPos));

        // Is potential occluder actually occluding the sample check
        occlusion += (occluderPos.z >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }

    ssaoValue = 1.0 - (occlusion / float(SSAO_SAMPLE_COUNT));  // Output the occlusion factor
}