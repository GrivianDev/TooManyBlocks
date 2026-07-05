#version 430 core

in vec2 screenUV;

layout(location = 0) out vec3 outColor;

uniform sampler2D u_opaquePassResult;
uniform sampler2D u_transparencyAccumTexture;
uniform sampler2D u_transparencyRevealTexture;

void main() {
    vec3 opaqueColor = texture(u_opaquePassResult, screenUV).rgb;
    vec4 accum = texture(u_transparencyAccumTexture, screenUV);
    float reveal = texture(u_transparencyRevealTexture, screenUV).r;

    // Safe normalization
    vec3 avgColor = accum.rgb / max(accum.a, 0.0001);
    vec3 finalColor = mix(opaqueColor, avgColor, (1.0 - reveal));
    outColor = vec3(finalColor);
}