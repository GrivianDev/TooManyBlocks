vec4 sampleTextureAtlas(
    sampler2D atlas,
    vec2 uv,
    uint textureIndex,
    uint texSize
) {
    ivec2 atlasSize = textureSize(atlas, 0);
    float textureScale = float(texSize) / float(atlasSize.x);
    float texturesPerRow = float(atlasSize.x) / float(texSize);
    vec2 index = vec2(mod(float(textureIndex), texturesPerRow), floor(float(textureIndex) / texturesPerRow));
    vec2 atlasUV = (index + uv) * textureScale;

    return texture(atlas, atlasUV);
}

vec4 sampleTextureAtlasFlippedY(
    sampler2D atlas,
    vec2 uv,
    uint textureIndex,
    uint texSize
) {
    vec2 adjustedUV = vec2(uv.x, 1.0 - uv.y);

    return sampleTextureAtlas(atlas, adjustedUV, textureIndex, texSize);
}