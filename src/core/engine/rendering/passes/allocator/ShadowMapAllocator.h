#ifndef TOOMANYBLOCKS_SHADOWMAPALLOCATOR_H
#define TOOMANYBLOCKS_SHADOWMAPALLOCATOR_H

#include <glm/vec2.hpp>
#include <vector>

class ShadowMapAllocator {
private:
    struct Atlas {
        std::vector<uint32_t> rows;
    };

    std::vector<Atlas> m_atlases;
    std::vector<Atlas> m_allocTestAtlases;

    unsigned int m_atlasSize;
    unsigned int m_tileSize;
    unsigned int m_gridSize;

    /**
     * Checks whether a square tile region is completely free.
     *
     * @param atlas Atlas occupancy bitmap.
     * @param x Left tile coordinate.
     * @param y Top tile coordinate.
     * @param size Region size in tiles.
     * @return True if the region can be allocated.
     */
    bool canFit(const Atlas& atlas, unsigned int x, unsigned int y, unsigned int size) const;

    /**
     * Marks a square tile region as occupied.
     *
     * @param atlas Target atlas.
     * @param x Left tile coordinate.
     * @param y Top tile coordinate.
     * @param size Region size in tiles.
     */
    void occupy(Atlas& atlas, unsigned int x, unsigned int y, unsigned int size);

    /**
     * Restores the temporary allocation state from the committed atlases.
     * Used before simulating a new allocation request.
     */
    void resetTestState();

public:
    struct Allocation {
        unsigned int atlasIndex;

        glm::vec2 atlasOffset;  // normalized
        glm::vec2 atlasScale;   // normalized

        unsigned int resolution;
    };

    ShadowMapAllocator() = default;

    ShadowMapAllocator(unsigned int atlasCount, unsigned int atlasSize, unsigned int minimumTileSize);

    /**
     * Clears all atlas allocations.
     */
    void clear();

    /**
     * Allocates one or more equally sized shadow maps.
     *
     * - Simulates allocation first.
     * - Commits only if every map fits.
     * - Maps may span multiple atlases.
     *
     * @param resolution Shadow map resolution.
     * @param count Number of maps.
     * @param output Allocation results.
     * @return True if all maps were allocated.
     */
    bool allocate(unsigned int resolution, unsigned int count, std::vector<Allocation>& output);

    /**
     * Largest uniform shadow map resolution fitting N maps entirely
     * inside a single atlas.
     *
     * @param mapCount Number of shadow maps.
     * @return Maximum supported resolution, or 0.
     */
    unsigned int getMaxUniformResolutionInAtlas(unsigned int mapCount) const;

    inline size_t atlasCount() const { return m_atlases.size(); }

    inline size_t atlasSize() const { return m_atlasSize; }
};

#endif
