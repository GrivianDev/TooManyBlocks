#ifndef TOOMANYBLOCKS_STATICMESHLOADER_H
#define TOOMANYBLOCKS_STATICMESHLOADER_H

#include <memory>
#include <string>

#include "engine/rendering/RenderData.h"
#include "engine/rendering/Vertices.h"
#include "engine/assets/cpu/CPURenderData.h"

CPURenderData<Vertex> loadStaticMeshFromObjFile(const std::string& objFilePath, bool flipWinding = false);

std::shared_ptr<RenderData> packToRenderData(const CPURenderData<Vertex>& data);

#endif
