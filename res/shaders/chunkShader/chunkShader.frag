#version 430 core

#include "../include/lights.glsl"

in vec3 position;
flat in uint texIndex;
in vec2 uv;
flat in vec3 normal;

layout(location = 0) out vec4 outColor;

uniform mat4 u_view;

uniform sampler2D u_textureAtlas;
uniform uint u_textureAtlasSize;
uniform uint u_textureSize;
uniform vec3 u_cameraPosition;

uniform bool u_ssaoEnabled;
uniform sampler2D u_ssaoTexture;
uniform uvec2 u_screenResolution;

vec4 sampleFromTexAtlas(vec2 uv_coord) {
    float textureScale = float(u_textureSize) / float(u_textureAtlasSize);
    float texturesPerRow = float(u_textureAtlasSize) / float(u_textureSize);

    vec2 index = vec2(mod(float(texIndex), texturesPerRow), floor(float(texIndex) / texturesPerRow));

    vec2 atlasUV = (index + uv_coord) * textureScale;
    return texture(u_textureAtlas, atlasUV);
}

void main() {
    vec2 uv_frag = fract(uv); // Effectively modulo for repeating texture on faces larger 1

    // Gradual fade to black for distant elements
    float dist = length(u_cameraPosition - position);
    float dropoffStartDistance = 50.0;
    float fadeDistance = 32.0;

    float fade = 1.0;
    if(dist > dropoffStartDistance) {
        fade = 1.0 - smoothstep(dropoffStartDistance, dropoffStartDistance + fadeDistance, dist);
        if(fade <= 0.0)
            discard;
    }

    // Sample the texture atlas
    vec4 texColor = sampleFromTexAtlas(uv_frag);
    // Discard non full alpha pixels
    if(texColor.a < 0.99)
        discard;

    vec3 color = texColor.rgb;

    // Initialize shadow factor
    vec3 lightContrib = accumulateLightContributions(u_view, u_cameraPosition, position, normal);

    vec2 screenUV = gl_FragCoord.xy / vec2(u_screenResolution);
    float occlusion = 1.0f;
    if (u_ssaoEnabled) {
        occlusion = texture(u_ssaoTexture, screenUV).r;  // SSAO value [0, 1]
    }
    color = ((0.15 * color) + (0.85 * clamp(lightContrib * color, 0.0, 1.0))) * occlusion;
    outColor = vec4(color * fade, 1.0);
}