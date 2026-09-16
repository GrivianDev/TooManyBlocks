vec4 transformPosition(mat4 matrix, vec3 position) {
    return matrix * vec4(position, 1.0);
}

vec3 transformPosition3D(mat4 matrix, vec3 position) {
    return (matrix * vec4(position, 1.0)).xyz;
}

vec3 transformNormal(mat4 matrix, vec3 normal) {
    return normalize(transpose(inverse(mat3(matrix))) * normal);
}