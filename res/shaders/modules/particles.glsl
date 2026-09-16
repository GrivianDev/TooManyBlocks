#define TEXINDEX_BITMASK  0xFFFFu
#define TEXINDEX_OFFSET   16u

#define GET_BITS(target, bitmask, position) ((target >> position) & bitmask)

vec3 calculateVertexOffset(vec2 vertexPosition, float size, vec3 cameraRight, vec3 cameraUp) {
    return (cameraRight * vertexPosition.x + cameraUp * vertexPosition.y) * size;
}

uint getParticleTexIndex(uint metadata) {
    return GET_BITS(metadata, TEXINDEX_BITMASK, TEXINDEX_OFFSET);
}