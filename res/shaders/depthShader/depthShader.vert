#version 430 core

layout(location = 0) in vec3 v_position;

uniform mat4 u_mvp;

void main() {
	gl_Position = u_mvp * vec4(v_position, 1.0);
}