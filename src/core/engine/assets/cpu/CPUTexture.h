#ifndef TOOMANYBLOCKS_CPUTEXTURE_H
#define TOOMANYBLOCKS_CPUTEXTURE_H

#include <memory>

struct CPUTexture {
    int width;
    int height;
    int channels;
    std::unique_ptr<unsigned char, void(*)(void*)> textureData;
};

#endif
