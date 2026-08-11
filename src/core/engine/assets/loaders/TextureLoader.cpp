#include "TextureLoader.h"

#include <stdexcept>
#include <stb_image.h>
#include "Logger.h"

#define DESIRED_CHANNELS 4

CPUTexture loadTextureFromFile(const std::string& texturePath) {
    int width;
    int height;
    int channels;
    unsigned char* data = stbi_load(texturePath.c_str(), &width, &height, &channels, DESIRED_CHANNELS);

    return {
        width,
        height,
        channels,
        std::unique_ptr<unsigned char, void(*)(void*)>(data, stbi_image_free)
    };
}
