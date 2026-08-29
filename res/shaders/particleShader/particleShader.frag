#version 430

in vec2 uv;
flat in vec4 color;
flat in uint texIndex;
flat in float timeToLive;

layout(location = 0) out vec4 outColor;

uniform bool u_useTexture;
uniform sampler2D u_textureAtlas;
uniform uint u_textureAtlasSize;
uniform uint u_textureSize;

vec4 sampleFromTexAtlas(vec2 uv_coord) {
    float textureScale = float(u_textureSize) / float(u_textureAtlasSize);
    float texturesPerRow = float(u_textureAtlasSize) / float(u_textureSize);

    vec2 index = vec2(mod(float(texIndex), texturesPerRow), floor(float(texIndex) / texturesPerRow));

    vec2 adjustedUV = vec2(uv_coord.x, 1.0 - uv_coord.y); // UVs are y-flipped
    vec2 atlasUV = (index + adjustedUV) * textureScale;
    return texture(u_textureAtlas, atlasUV);
}

void main() {
    if(timeToLive <= 0.0) {
        discard; // Discard pixels that belong to dead particles
    }

    vec4 fragColor = vec4(0.0);

    if(u_useTexture) {
        fragColor = sampleFromTexAtlas(uv).rgba;
        fragColor *= color;
    } else {
        fragColor = color;
    }

    // Discard non full alpha pixels
    if(fragColor.a < 0.99)
        discard;
    outColor = vec4(fragColor.rgb, 1.0);
}