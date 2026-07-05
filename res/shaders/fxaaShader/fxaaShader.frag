#version 430 core

in vec2 screenUV;
out vec4 outColor;

uniform sampler2D u_inputTexture;
uniform vec2 u_texelSize; // 1.0 / resolution

const float FXAA_SPAN_MAX = 8.0;
const float FXAA_REDUCE_MUL = 1.0 / 8.0;
const float FXAA_REDUCE_MIN = 1.0 / 128.0;

// Luma factor (perceptual brightness)
const vec3 luma = vec3(0.299, 0.587, 0.114);

void main() {
    // Sample center and 4 diagonal neighbors
    vec3 rgbM = texture(u_inputTexture, screenUV).rgb;
    vec3 rgbNW = textureOffset(u_inputTexture, screenUV, ivec2(-1, -1)).rgb;
    vec3 rgbNE = textureOffset(u_inputTexture, screenUV, ivec2(1, -1)).rgb;
    vec3 rgbSW = textureOffset(u_inputTexture, screenUV, ivec2(-1, 1)).rgb;
    vec3 rgbSE = textureOffset(u_inputTexture, screenUV, ivec2(1, 1)).rgb;

    // Compute luma for edge detection
    float lumaNW = dot(rgbNW, luma);
    float lumaNE = dot(rgbNE, luma);
    float lumaSW = dot(rgbSW, luma);
    float lumaSE = dot(rgbSE, luma);
    float lumaM = dot(rgbM, luma);

    // Find contrast range
    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    // Estimate edge direction from luma
    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y = ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    // Reduce direction based on contrast
    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);

    // Normalize edge direction to sampling range
    dir = clamp(dir * rcpDirMin, vec2(-FXAA_SPAN_MAX), vec2(FXAA_SPAN_MAX)) * u_texelSize;

    vec3 rgbA = 0.5 * (texture(u_inputTexture, screenUV + dir * (1.0 / 3.0 - 0.5)).rgb +
        texture(u_inputTexture, screenUV + dir * (2.0 / 3.0 - 0.5)).rgb);

    vec3 rgbB = rgbA * 0.5 +
        0.25 * (texture(u_inputTexture, screenUV + dir * (0.0 / 3.0 - 0.5)).rgb +
        texture(u_inputTexture, screenUV + dir * (3.0 / 3.0 - 0.5)).rgb);

    float lumaB = dot(rgbB, luma);

    // Choose between sharper or smoother result
    outColor = (lumaB < lumaMin || lumaB > lumaMax) ? vec4(rgbA, 1.0) : vec4(rgbB, 1.0);
}