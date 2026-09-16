#define MAX_JOINTS 100

layout(std140) uniform JointMatrices {
    mat4 u_jointMatrices[MAX_JOINTS];
};

mat4 calculateSkinMatrix(uvec4 jointIndices, vec4 jointWeights) {
    // Each vertex is influenced by up to 4 joints
    return jointWeights.x * u_jointMatrices[jointIndices.x] +
        jointWeights.y * u_jointMatrices[jointIndices.y] +
        jointWeights.z * u_jointMatrices[jointIndices.z] +
        jointWeights.w * u_jointMatrices[jointIndices.w];
}