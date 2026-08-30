#ifndef TOOMANYBLOCKS_PERLINOISE_H
#define TOOMANYBLOCKS_PERLINOISE_H

#include <memory>
#include <vector>

class PerlinNoise {
private:
    const uint32_t m_seed;

public:
    /**
     * @brief Constructor for the PerlinNoise class.
     *
     * @param seed The seed value for generating random gradients. Different seeds will produce different noise
     * patterns.
     */
    PerlinNoise(uint32_t seed) : m_seed(seed) {};

    /**
     * @brief Generates a Perlin noise map for a given region.
     *
     * @param regionSize Dimensions of the noise map (e.g., {width, height} for 2D or {width, height, depth} for 3D).
     * @param regionOffset Starting offsets in each dimension to allow subregion generation.
     * @param baseSubsectionSize Size of the base gradient grid cells (must be a power of two and at least 2).
     * @param octaves Number of noise layers to combine, increasing detail with each layer.
     * @param amplitude Initial amplitude (magnitude) multiplier for the noise, controlling contrast.
     * @param persistence Rate at which amplitude decreases with each octave (values in [0, 1] give natural results).
     * @return A `std::unique_ptr<float[]>` pointing to a 1D array of floats in [0.0f, 1.0f] representing the noise map.
     *
     * @note The array is 1D but represents an N-dimensional map. For a coordinate (x, y, ...):
     *       `index = x + (y * width) + (z * width * height)` for 3D.
     */
    std::unique_ptr<float[]> generatePerlinNoise(
        const std::vector<int>& regionSize,
        const std::vector<int>& regionOffset,
        int baseSubsectionSize = 256,
        int octaves = 1,
        float amplitude = 1.0f,
        float persistence = 0.5f
    );

    inline uint32_t seed() const { return m_seed; };
};

#endif