#ifndef TOOMANYBLOCKS_BONEMASK_H
#define TOOMANYBLOCKS_BONEMASK_H

#include <algorithm>
#include <vector>

#include "engine/assets/cpu/CPUSkeletalMeshData.h"

class BoneMask {
private:
    std::vector<float> m_weights;

public:
    BoneMask(size_t nodeCount);

    inline void setWeight(int nodeIndex, float weight) { m_weights[nodeIndex] = std::clamp(weight, 0.0f, 1.0f); };
    void setWeight(const std::vector<Node>& nodes, const std::string& nodeName, float weight);
    inline void setAll(float weight) { std::fill(m_weights.begin(), m_weights.end(), std::clamp(weight, 0.0f, 1.0f)); }
    inline const std::vector<float>& getWeights() const { return m_weights; };

    void setHierarchyWeight(const std::vector<Node>& nodes, int rootNode, float weight = 1.0f);
    void setHierarchyWeight(const std::vector<Node>& nodes, const std::string& rootNodeName, float weight = 1.0f);

    static BoneMask hierarchy(
        const std::vector<Node>& nodes,
        int rootNode,
        float hierarchyWeight = 1.0f,
        float defaultWeight = 0.0f
    );
    static BoneMask hierarchy(
        const std::vector<Node>& nodes,
        const std::string& rootNodeName,
        float hierarchyWeight = 1.0f,
        float defaultWeight = 0.0f
    );
    static BoneMask wholeBody(size_t nodeCount, float weight = 1.0f);

    // Mask algebra
    BoneMask& operator+=(const BoneMask& other);
    BoneMask& operator-=(const BoneMask& other);
    BoneMask& operator*=(const BoneMask& other);
    BoneMask& operator|=(const BoneMask& other);
    BoneMask& operator&=(const BoneMask& other);

    BoneMask operator+(const BoneMask& other) const;
    BoneMask operator-(const BoneMask& other) const;
    BoneMask operator*(const BoneMask& other) const;
    BoneMask operator|(const BoneMask& other) const;
    BoneMask operator&(const BoneMask& other) const;

    BoneMask& operator*=(float scalar);
    BoneMask operator*(float scalar) const;
    BoneMask& operator+=(float scalar);
    BoneMask operator+(float scalar) const;

    BoneMask operator~() const;
};

#endif
