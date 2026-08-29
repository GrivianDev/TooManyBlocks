#version 430

#define MAX_JOINTS 100

layout(location = 0) in vec3 v_position;
layout(location = 3) in uvec4 v_jointIndices;
layout(location = 4) in vec4 v_jointWeights;

layout(std140) uniform JointMatrices {
    mat4 u_jointMatrices[MAX_JOINTS];
};

uniform mat4 u_mvp;

void main() {
    // Each vertex is influenced by up to 4 joints
    mat4 skinMatrix = 
        v_jointWeights.x * u_jointMatrices[v_jointIndices.x] +
        v_jointWeights.y * u_jointMatrices[v_jointIndices.y] +
        v_jointWeights.z * u_jointMatrices[v_jointIndices.z] +
        v_jointWeights.w * u_jointMatrices[v_jointIndices.w];

    vec4 skinnedPosition = skinMatrix * vec4(v_position, 1.0);

    // Apply model view projection after skinning
    gl_Position = u_mvp * skinnedPosition;
}