#version 430 core

layout(location = 0) in vec3 v_position;
layout(location = 1) in vec2 v_uv;
layout(location = 2) in vec3 v_normal;

out vec3 position;
out vec2 uv;
out vec3 normal;

uniform mat4 u_mvp;
uniform mat4 u_model;

void main() {
	gl_Position = u_mvp * vec4(v_position, 1.0);
	position = (u_model * vec4(v_position, 1.0)).xyz;
	uv = v_uv;
	normal = normalize(mat3(u_model) * v_normal);
}