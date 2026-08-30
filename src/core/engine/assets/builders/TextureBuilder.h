#ifndef TOOMANYBLOCKS_TEXTUREBUILDER_H
#define TOOMANYBLOCKS_TEXTUREBUILDER_H

#include "engine/assets/cpu/CPUTexture.h"
#include "engine/rendering/opengl/Texture.h"
#include "foundation/threading/Future.h"

Future<Texture> build(const Future<CPUTexture>& cpuTexture);

#endif
