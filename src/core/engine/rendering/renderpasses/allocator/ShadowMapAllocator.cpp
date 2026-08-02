#include "ShadowMapAllocator.h"

#include <stdexcept>

struct TmpAllocation {
    unsigned int atlasIndex;

    glm::uvec2 tileOffset;
};

bool ShadowMapAllocator::canFit(const Atlas& atlas, unsigned int x, unsigned int y, unsigned int size) const {
    if (x + size > m_gridSize || y + size > m_gridSize) return false;

    uint32_t mask = ((1u << size) - 1u) << x;

    for (unsigned int row = y; row < y + size; row++) {
        if (atlas.rows[row] & mask) return false;
    }

    return true;
}

void ShadowMapAllocator::occupy(Atlas& atlas, unsigned int x, unsigned int y, unsigned int size) {
    uint32_t mask = ((1u << size) - 1u) << x;

    for (unsigned int row = y; row < y + size; row++) {
        atlas.rows[row] |= mask;
    }
}

void ShadowMapAllocator::resetTestState() { m_allocTestAtlases = m_atlases; }

ShadowMapAllocator::ShadowMapAllocator(unsigned int atlasCount, unsigned int atlasSize, unsigned int minimumTileSize)
    : m_atlasSize(atlasSize), m_tileSize(minimumTileSize), m_gridSize(atlasSize / minimumTileSize) {
    if (m_gridSize > 32) throw std::runtime_error("Grid too large for uint32 row representation");

    m_atlases.resize(atlasCount);
    m_allocTestAtlases.resize(atlasCount);

    for (Atlas& atlas : m_atlases) {
        atlas.rows.resize(m_gridSize, 0);
    }
    for (Atlas& atlas : m_allocTestAtlases) {
        atlas.rows.resize(m_gridSize, 0);
    }
}

void ShadowMapAllocator::clear() {
    for (Atlas& atlas : m_atlases) {
        std::fill(atlas.rows.begin(), atlas.rows.end(), 0);
    }
}

bool ShadowMapAllocator::allocate(unsigned int resolution, unsigned int count, std::vector<Allocation>& output) {
    if (resolution % m_tileSize != 0) throw std::runtime_error("Resolution not a multiple of shadow map tile size");

    const unsigned int cells = resolution / m_tileSize;
    const float invAtlasSize = 1.0f / static_cast<float>(m_atlasSize);

    std::vector<TmpAllocation> allocations;
    allocations.reserve(count);

    resetTestState();

    bool allocationComplete = false;
    for (unsigned int atlasIndex = 0; atlasIndex < m_allocTestAtlases.size() && !allocationComplete; atlasIndex++) {
        Atlas& atlas = m_allocTestAtlases[atlasIndex];

        for (unsigned int y = 0; y <= m_gridSize - cells && !allocationComplete; y += cells) {
            for (unsigned int x = 0; x <= m_gridSize - cells; x += cells) {
                if (!canFit(atlas, x, y, cells)) continue;

                occupy(atlas, x, y, cells);

                allocations.push_back({atlasIndex, {x, y}});
                if (allocations.size() == count) {
                    allocationComplete = true;
                    break;
                }
            }
        }
    }

    if (!allocationComplete) return false;

    for (TmpAllocation allocation : allocations) {
        occupy(m_atlases[allocation.atlasIndex], allocation.tileOffset.x, allocation.tileOffset.y, cells);

        Allocation newAlloc;
        newAlloc.atlasIndex = allocation.atlasIndex;
        newAlloc.atlasOffset = {
            static_cast<float>(allocation.tileOffset.x * m_tileSize) * invAtlasSize,
            static_cast<float>(allocation.tileOffset.y * m_tileSize) * invAtlasSize
        };

        const float padding = 2.0f;
        newAlloc.atlasScale = {(resolution - padding) * invAtlasSize, (resolution - padding) * invAtlasSize};
        newAlloc.resolution = resolution;
        output.push_back(newAlloc);
    }
    return true;
}

unsigned int ShadowMapAllocator::getMaxUniformResolutionInAtlas(unsigned int mapCount) const {
    if (mapCount == 0) return 0;

    unsigned int resolution = m_atlasSize;
    while (resolution > 1) {
        unsigned int perRow = atlasSize() / resolution;
        if (perRow * perRow >= mapCount) return resolution;
        resolution /= 2;
    }

    return 0;
}
