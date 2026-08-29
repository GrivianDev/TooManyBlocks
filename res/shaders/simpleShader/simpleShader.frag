#version 430 core

#include "../include/lights.glsl"

in vec3 position;
in vec2 uv;
in vec3 normal;

layout(location = 0) out vec4 outColor;

uniform mat4 u_view;
uniform vec3 u_cameraPosition;

uniform vec3 u_color;
uniform bool u_useTexture;
uniform sampler2D u_texture;

void main() {
	vec4 fragColor = u_useTexture ? texture(u_texture, uv) : vec4(u_color, 1.0);
	if(fragColor.a < 0.99)
		discard;

	vec3 lightContrib = accumulateLightContributions(u_view, u_cameraPosition, position, normal);
	vec3 color = (0.15 * fragColor.rgb) + (0.85 * clamp(lightContrib * fragColor.rgb, 0.0, 1.0));
	outColor = vec4(color, 1.0);
}