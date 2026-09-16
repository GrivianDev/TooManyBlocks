uniform sampler2D u_ssaoTexture;

float sampleSSAO(uvec2 screenResolution) {
    vec2 screenUV = gl_FragCoord.xy / vec2(screenResolution);
    return texture(u_ssaoTexture, screenUV).r;
}