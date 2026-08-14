#version 430 core

layout(location = 0) in vec2 v_position;  // 2D Screen Position
layout(location = 1) in vec2 v_uv; // Screen UV

out vec2 screenUV;

void main() {
    screenUV = v_uv;
    gl_Position = vec4(v_position, 0.0, 1.0);
}