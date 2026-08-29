#version 430

#include "../include/lights.glsl"

in vec3 position;
in vec2 uv;
in vec3 normal;

layout(location = 0) out vec4 outColor;

uniform mat4 u_view;
uniform vec3 u_cameraPosition;

uniform sampler2D u_texture;

void main() {
    vec4 fragColor = texture(u_texture, uv).rgba;
    // Discard non full alpha pixels
    if (fragColor.a < 0.99)
        discard;

    vec3 lightContrib = accumulateLightContributions(u_view, u_cameraPosition, position, normal);
    vec3 color = (0.15 * fragColor.rgb) + (0.85 * clamp(lightContrib * fragColor.rgb, 0.0, 1.0));
    outColor = vec4(color, 1.0);
}