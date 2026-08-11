#ifndef TOOMANYBLOCKS_TEXTUREBUILDER_H
#define TOOMANYBLOCKS_TEXTUREBUILDER_H

#include "engine/rendering/lowlevelapi/Texture.h"
#include "engine/assets/cpu/CPUTexture.h"
#include "foundation/threading/Future.h"

Future<Texture> build(const Future<CPUTexture>& cpuTexture);

#endif
