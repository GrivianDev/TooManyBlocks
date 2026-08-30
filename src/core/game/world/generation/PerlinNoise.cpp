#include "PerlinNoise.h"

#include <cmath>
#include <stdexcept>

// Interpolation function
static constexpr float smoothstep(float t) { return t * t * (3.0f - 2.0f * t); }

// Checks if a value is a power of two by checking if it is a: positive and b: has only 1 bit set
static constexpr bool isPowerOfTwo(int n) { return n > 0 && (n & (n - 1)) == 0; }

static inline void calcNDVecFromFlatIndex(
    int* coordDest, int idx, const int* offsets, const int* sizeOfEachDimension, int dimensions
) {
    for (int d = 0; d < dimensions; d++) {
        int localIndex =
            idx % sizeOfEachDimension[d];  // Get the local index within the current dimension's gradient count
        if (offsets != nullptr) {
            coordDest[d] = offsets[d] + localIndex;  // Compute the global coordinate for this dimension
        } else {
            coordDest[d] = localIndex;
        }
        idx /=
            sizeOfEachDimension[d];  // Move to the next dimension by dividing by the gradient count of this dimension
    }
}

// Lightweight + platform independent replacement of rand()
static inline uint32_t xorshift32(uint32_t& state) {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

static inline void putRandomGradient(float* gradientDest, const int* sourceCoords, int dimension, uint32_t seed) {
    // Combine coord values and seed into source hash
    uint32_t hash = seed;
    for (int i = 0; i < dimension; i++) {
        const float SCALE_FACTOR = 1e6f;
        hash ^= static_cast<uint32_t>(sourceCoords[i] * SCALE_FACTOR) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    }

    // Generate a random vector (each element in range [-1, 1])
    // Uses hash as inital rng state
    // Calculates magnitude alongside
    float magnitude = 0.0f;
    for (int i = 0; i < dimension; i++) {
        gradientDest[i] = static_cast<float>(xorshift32(hash)) / UINT32_MAX * 2.0f - 1.0f;
        magnitude += gradientDest[i] * gradientDest[i];
    }
    magnitude = std::sqrt(magnitude);

    if (magnitude == 0.0f) {
        // Handle zero magnitude by generating a default normalized vector
        float value = 1.0f / std::sqrt(static_cast<float>(dimension));
        for (int i = 0; i < dimension; i++) {
            gradientDest[i] = value;
        }
    } else {
        // Valid vector, now normalize
        for (int i = 0; i < dimension; i++) {
            gradientDest[i] /= magnitude;
        }
    }
}

std::unique_ptr<float[]> PerlinNoise::generatePerlinNoise(
    const std::vector<int>& regionSize,
    const std::vector<int>& regionOffset,
    int baseSubsectionSize,
    int octaves,
    float amplitude,
    float persistence
) {
    int dimensions = static_cast<int>(regionOffset.size());
    if (dimensions < 1 || static_cast<int>(regionSize.size()) != dimensions) {
        throw std::runtime_error("Starts and extents must have the same non-zero dimensions.");
    }
    for (int d = 0; d < dimensions; d++) {
        if (regionSize[d] < 1) throw std::runtime_error("All noise map dimensions must be greater than zero.");
    }
    if (amplitude < 0) throw std::runtime_error("Amplitude must be non-negative.");
    if (persistence < 0) throw std::runtime_error("Persistence must be non-negative.");
    if (baseSubsectionSize <= 1) throw std::runtime_error("Base subdivision size must be greater 1");
    if (!isPowerOfTwo(baseSubsectionSize)) throw std::runtime_error("Base subdivision size must a power of two");
    if (octaves < 1) throw std::runtime_error("Octaves must be greater zero");

    // ############## Precompute values ###################
    // Raw manually allocated arrays to store precomputed values for each octave for least overhead and efficiency
    float** octaveGradientVectors =
        new float*[octaves];  // Per octave you have different amounts of vectors (!!! Here vectors of same octave are
                              // layed out in sequence in memory !!!)
    int* octaveTotalGradients = new int[octaves];
    int* octaveSubsectionSizes = new int[octaves];
    int** octaveGradientOffsets = new int*[octaves];
    int** octaveGradCountsPerDim = new int*[octaves];  // Total number of gradients along each dimension per octave
    int** octaveLocalPixelOffsets = new int*[octaves];
    for (int oi = 0; oi < octaves; oi++) {
        octaveGradientOffsets[oi] = new int[dimensions];
        octaveGradCountsPerDim[oi] = new int[dimensions];
        octaveLocalPixelOffsets[oi] = new int[dimensions];
    }
    float* octaveAmplitudeScales = new float[octaves];
    float maxTotalAmplitude = 0.0f;

    int* tmpCoordBuff = new int[dimensions];

    // Generate info for each octave and most importantly
    // only generate relevant gradient vectors.
    int validOctaveCount =
        octaves;  // Track how many "relevant" octaves there are (octaves whose subsections are larger than 1x1 units)
    for (int oi = 0; oi < octaves; oi++) {
        const int currentSubsectionSize =
            baseSubsectionSize / (1 << oi);  // Half each subsection size for each octave (Increase frequency)

        if (currentSubsectionSize <= 1) {
            validOctaveCount = oi;
            break;  // Exit the loop as further octaves will not be meaningful
        }

        octaveSubsectionSizes[oi] = currentSubsectionSize;
        octaveAmplitudeScales[oi] =
            amplitude * std::pow(
                            persistence, static_cast<float>(oi)
                        );  // Exponential dropoff [persistence < 1.0f] or increase [persistence > 1.0f]

        // Compute gradient grid offsets for each dimension
        int totalGradCount = 1;  // Total number of gradients for this octave
        for (int d = 0; d < dimensions; d++) {
            octaveLocalPixelOffsets[oi][d] = regionOffset[d] % currentSubsectionSize;
            if (octaveLocalPixelOffsets[oi][d] < 0) {
                octaveLocalPixelOffsets[oi][d] += currentSubsectionSize;
            }

            int gradientStartIdx = regionOffset[d] / currentSubsectionSize;
            if (regionOffset[d] < 0 && regionOffset[d] % currentSubsectionSize != 0) {
                gradientStartIdx--;
            }
            octaveGradientOffsets[oi][d] = gradientStartIdx;

            int regionEnd = regionOffset[d] + regionSize[d] - 1;  // Last pixel in region
            int gradientEndIdx = regionEnd / currentSubsectionSize;
            if (regionEnd < 0 && regionEnd % currentSubsectionSize != 0) {
                gradientEndIdx--;
            }

            gradientEndIdx++;  // Include the gradient at the far-right edge of the region

            octaveGradCountsPerDim[oi][d] = (gradientEndIdx - gradientStartIdx) + 1;
            totalGradCount *= octaveGradCountsPerDim[oi][d];
        }
        octaveTotalGradients[oi] = totalGradCount;

        // Allocate space for gradient vectors
        octaveGradientVectors[oi] = new float[totalGradCount * dimensions];

        // Generate gradients for this octave
        for (int gi = 0; gi < totalGradCount; gi++) {
            calcNDVecFromFlatIndex(tmpCoordBuff, gi, octaveGradientOffsets[oi], octaveGradCountsPerDim[oi], dimensions);
            for (int d = 0; d < dimensions; d++) {
                tmpCoordBuff[d] *= currentSubsectionSize;  // Map to global coordinates
            }
            putRandomGradient(
                octaveGradientVectors[oi] + (gi * dimensions) /* Place vector at correct location in memory */,
                tmpCoordBuff, dimensions, m_seed
            );
        }
    }
    // Precalculate maximal total amplitude after combining octaves (for normalization)
    for (int oi = 0; oi < octaves; oi++) {
        maxTotalAmplitude += octaveAmplitudeScales[oi];
    }

    // ################ End precomputation ################

    float* noiseMap = nullptr;

    // Only create noise map if at least one octave with subsectionSize greater 1x1 exists
    if (validOctaveCount != 0) {
        // Calculate number of noise points to generate
        int totalNoisePoints = 1;
        for (const int extent : regionSize) {
            totalNoisePoints *= extent;
        }

        noiseMap = new float[totalNoisePoints];

        // Preallocate buffers for noise calculation for repeated access during per pixel operations
        int cornerCount = 1 << dimensions;
        int* localCellCoord = new int[dimensions];  // Local coordinates within the current grid cell
        int* gridCoord = new int[dimensions];       // Indices of the grid cell in gradient space
        int* cornerCoord = new int[dimensions];
        float* localCellCoordAsFraction = new float[dimensions];  // Fraction in [0, 1]
        float* interpolationFactors = new float[dimensions];
        float* cornerDotProducts = new float[cornerCount];

        // Calculate noise map values
        for (int i = 0; i < totalNoisePoints; i++) {
            calcNDVecFromFlatIndex(tmpCoordBuff, i, nullptr, regionSize.data(), dimensions);
            float total = 0.0f;

            for (int oi = 0; oi < validOctaveCount; oi++) {
                // ##################### Noise point calculation ########################
                // Constants for calculation
                const int subsectionSize = octaveSubsectionSizes[oi];
                const float amplitudeScale = octaveAmplitudeScales[oi];
                const float* gradients = octaveGradientVectors[oi];
                const int* gradCounts = octaveGradCountsPerDim[oi];
                const int* localOffset = octaveLocalPixelOffsets[oi];

                for (int d = 0; d < dimensions; d++) {
                    localCellCoord[d] = (tmpCoordBuff[d] + localOffset[d]) &
                                        (subsectionSize - 1);  // Fast modulo (subsectionSize is powers of 2)
                    // Determine relative grid cell coordinate based on the subsection size (From first generated
                    // gradient)
                    gridCoord[d] = (tmpCoordBuff[d] + localOffset[d]) / subsectionSize;
                    localCellCoordAsFraction[d] = static_cast<float>(localCellCoord[d]) / subsectionSize;

                    // Smooth interpolation factors using fade function
                    interpolationFactors[d] = smoothstep(localCellCoordAsFraction[d]);
                }

                for (int corner = 0; corner < cornerCount; corner++) {
                    // Compute the coordinates of the current corner
                    int gradientIndex = 0;

                    // Calculate the flattened index for this corner
                    int stride = 1;
                    for (int d = 0; d < dimensions; d++) {
                        cornerCoord[d] = gridCoord[d] + ((corner >> d) & 1);  // Add corner offset
                        // Retrieve the gradient vector for the corner based on flattened index
                        gradientIndex += cornerCoord[d] * stride;
                        stride *= gradCounts[d];  // Update stride for each dimension
                    }

                    const float* gradient =
                        gradients + (gradientIndex * dimensions);  // Access vector at correct location in memory

                    // Compute the dot product between gradient and local offset
                    float dotProduct = 0.0f;
                    for (int d = 0; d < dimensions; d++) {
                        float offset = localCellCoordAsFraction[d] - ((corner >> d) & 1);  // Offset from corner
                        dotProduct += gradient[d] * offset;
                    }
                    cornerDotProducts[corner] = dotProduct;
                }

                // Perform interpolation across all dot products
                for (int dim = dimensions - 1; dim >= 0; dim--) {
                    int step = 1 << dim;
                    for (int s = 0; s < step; s++) {
                        cornerDotProducts[s] =
                            cornerDotProducts[s] +
                            interpolationFactors[dim] * (cornerDotProducts[s + step] - cornerDotProducts[s]);
                    }
                }
                // Add result
                total += cornerDotProducts[0] * amplitudeScale;
                // #################### End noise point calculation #####################
            }

            noiseMap[i] = (total / maxTotalAmplitude + 1.0f) * 0.5f;  // Map [-1, 1] to [0, 1]
        }

        // Cleanup per pixel buffers
        delete[] localCellCoord;
        delete[] gridCoord;
        delete[] cornerCoord;
        delete[] localCellCoordAsFraction;
        delete[] interpolationFactors;
        delete[] cornerDotProducts;
    }

    // Cleanup precomputation buffers
    for (int oi = 0; oi < octaves; oi++) {
        delete[] octaveGradientVectors[oi];
        delete[] octaveGradientOffsets[oi];
        delete[] octaveGradCountsPerDim[oi];
        delete[] octaveLocalPixelOffsets[oi];
    }
    delete[] octaveGradientVectors;
    delete[] octaveTotalGradients;
    delete[] octaveSubsectionSizes;
    delete[] octaveGradientOffsets;
    delete[] octaveGradCountsPerDim;
    delete[] octaveAmplitudeScales;
    delete[] octaveLocalPixelOffsets;

    delete[] tmpCoordBuff;

    // Return with custom deleter for dynamic array, ensuring proper cleanup for array.
    return std::unique_ptr<float[]>(noiseMap, std::default_delete<float[]>());
}