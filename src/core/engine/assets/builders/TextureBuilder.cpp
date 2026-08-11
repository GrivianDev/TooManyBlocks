#include "TextureBuilder.h"

Future<Texture> build(const Future<CPUTexture>& cpuTexture) {
    Future<Texture> textureFuture(
        [cpuTexture] {
            const CPUTexture& cpuTex = cpuTexture.value();
            return Texture::create(
                TextureType::Color, cpuTex.width, cpuTex.height, cpuTex.channels, cpuTex.textureData.get()
            );
        },
        DEFAULT_TASKCONTEXT,
        Executor::Main
    );
    return textureFuture.dependsOn(cpuTexture).start();
}