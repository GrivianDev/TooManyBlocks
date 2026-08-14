#version 430 core

in vec2 screenUV;
out vec4 outColor;

uniform sampler2D u_inputTexture;

void main() {
    outColor = vec4(texture(u_inputTexture, screenUV).rgb, 1.0);
}