#ifndef TOOMANYBLOCKS_SHADERMANAGER_H
#define TOOMANYBLOCKS_SHADERMANAGER_H

#include <unordered_map>

#include "engine/rendering/shaders/RenderProgram.h"
#include "engine/rendering/shaders/ShaderKey.h"
#include "engine/rendering/shaders/generator/modules/ShaderModuleRegistry.h"

class ShaderManager {
private:
    ShaderModuleRegistry m_modules;

    std::unordered_map<ShaderKey, RenderProgram, ShaderKeyHash> m_programs;

public:
    RenderProgram& loadProgram(const ShaderKey& key);

    void setup();

    inline size_t getProgramCount() const { return m_programs.size(); }
};

#endif
