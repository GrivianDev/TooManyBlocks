#define POSITION_BITMASK  0x3Fu
#define X_POSITION_OFFSET 26u
#define Y_POSITION_OFFSET 20u
#define Z_POSITION_OFFSET 14u

#define ANIMATION_FRAME_COUNT_BITMASK   0xFFu
#define ANIMATION_FRAME_COUNT_OFFSET    6
#define ANIMATION_FPS_BITMASK           0x3Fu
#define ANIMATION_FPS_OFFSET            0

#define TEXINDEX_BITMASK 0xFFu
#define TEXINDEX_OFFSET 16u

#define X_UV_BITMASK 0x3FFu
#define Y_UV_BITMASK 0x3Fu
#define X_UV_OFFSET 10u
#define Y_UV_OFFSET 4u

#define NORMAL_BITMASK 0x07u
#define NORMAL_OFFSET 0u

#define GET_BITS(target, bitmask, position) ((target >> position) & bitmask)

#define PositiveX 0u
#define NegativeX 1u
#define PositiveY 2u
#define NegativeY 3u
#define PositiveZ 4u
#define NegativeZ 5u

vec3 decodePosition(uint compressedData) {
    uvec3 pos;
    pos.x = GET_BITS(compressedData, POSITION_BITMASK, X_POSITION_OFFSET);
    pos.y = GET_BITS(compressedData, POSITION_BITMASK, Y_POSITION_OFFSET);
    pos.z = GET_BITS(compressedData, POSITION_BITMASK, Z_POSITION_OFFSET);
    return vec3(pos);
}

uint decodeTexIndex(uint compressedData) {
    return GET_BITS(compressedData, TEXINDEX_BITMASK, TEXINDEX_OFFSET);
}

vec2 decodeUV(uint compressedData) {
    float uvX = float(GET_BITS(compressedData, X_UV_BITMASK, X_UV_OFFSET));
    float uvY = float(GET_BITS(compressedData, Y_UV_BITMASK, Y_UV_OFFSET));
    return vec2(uvX, uvY);
}

vec3 decodeNormal(uint compressedData) {
    uint normalCode = GET_BITS(compressedData, NORMAL_BITMASK, NORMAL_OFFSET);
    switch (normalCode) {
        case PositiveX: return vec3(1.0, 0.0, 0.0);
        case NegativeX: return vec3(-1.0, 0.0, 0.0);
        case PositiveY: return vec3(0.0, 1.0,0.0);
        case NegativeY: return vec3(0.0, -1.0, 0.0);
        case PositiveZ: return vec3(0.0, 0.0, 1.0);
        case NegativeZ: return vec3(0.0, 0.0, -1.0);
        default: return vec3(0.0, 1.0, 0.0);
    }
}
