#include "BoneMask.h"

#include <stdexcept>

BoneMask::BoneMask(size_t nodeCount) : m_weights(nodeCount) {}

void BoneMask::setWeight(const std::vector<Node>& nodes, const std::string& nodeName, float weight) {
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].name == nodeName) {
            setWeight(static_cast<int>(i), weight);
            return;
        }
    }
}

void BoneMask::setHierarchyWeight(const std::vector<Node>& nodes, int rootNode, float weight) {
    if (rootNode < 0 || static_cast<size_t>(rootNode) >= nodes.size()) {
        return;
    }

    for (size_t i = 0; i < nodes.size(); i++) {
        int current = static_cast<int>(i);

        // Check if node inherits from specified node
        while (current >= 0) {
            if (current == rootNode) {
                setWeight(static_cast<int>(i), weight);
                break;
            }

            current = nodes[current].parentIndex;
        }
    }
}

void BoneMask::setHierarchyWeight(const std::vector<Node>& nodes, const std::string& rootNodeName, float weight) {
    for (size_t i = 0; i < nodes.size(); i++) {
        if (nodes[i].name == rootNodeName) {
            setHierarchyWeight(nodes, static_cast<int>(i), weight);
            return;
        }
    }
}

BoneMask BoneMask::hierarchy(const std::vector<Node>& nodes, int rootNode, float hierarchyWeight, float defaultWeight) {
    BoneMask mask(nodes.size());
    mask.setAll(defaultWeight);
    mask.setHierarchyWeight(nodes, rootNode, hierarchyWeight);
    return mask;
}

BoneMask BoneMask::hierarchy(
    const std::vector<Node>& nodes,
    const std::string& rootNodeName,
    float hierarchyWeight,
    float defaultWeight
) {
    BoneMask mask(nodes.size());
    mask.setAll(defaultWeight);
    mask.setHierarchyWeight(nodes, rootNodeName, hierarchyWeight);
    return mask;
}

BoneMask BoneMask::wholeBody(size_t nodeCount, float weight) {
    BoneMask mask(nodeCount);
    mask.setAll(weight);
    return mask;
}

BoneMask& BoneMask::operator+=(const BoneMask& other) {
    if (m_weights.size() != other.m_weights.size()) throw std::runtime_error("Operator on masks with different sizes");
    for (size_t i = 0; i < m_weights.size(); i++) {
        setWeight(i, m_weights[i] + other.getWeights()[i]);
    }
    return *this;
}

BoneMask& BoneMask::operator-=(const BoneMask& other) {
    if (m_weights.size() != other.m_weights.size()) throw std::runtime_error("Operator on masks with different sizes");
    for (size_t i = 0; i < m_weights.size(); i++) {
        setWeight(i, m_weights[i] - other.m_weights[i]);
    }
    return *this;
}

BoneMask& BoneMask::operator*=(const BoneMask& other) {
    if (m_weights.size() != other.m_weights.size()) throw std::runtime_error("Operator on masks with different sizes");
    for (size_t i = 0; i < m_weights.size(); i++) {
        setWeight(i, m_weights[i] * other.m_weights[i]);
    }
    return *this;
}

BoneMask& BoneMask::operator|=(const BoneMask& other) {
    if (m_weights.size() != other.m_weights.size()) throw std::runtime_error("Operator on masks with different sizes");
    for (size_t i = 0; i < m_weights.size(); i++) {
        m_weights[i] = std::max(m_weights[i], other.m_weights[i]);
    }
    return *this;
}

BoneMask& BoneMask::operator&=(const BoneMask& other) {
    if (m_weights.size() != other.m_weights.size()) throw std::runtime_error("Operator on masks with different sizes");
    for (size_t i = 0; i < m_weights.size(); i++) {
        m_weights[i] = std::min(m_weights[i], other.m_weights[i]);
    }
    return *this;
}

BoneMask BoneMask::operator+(const BoneMask& other) const {
    BoneMask result = *this;
    return result += other;
}

BoneMask BoneMask::operator-(const BoneMask& other) const {
    BoneMask result = *this;
    return result -= other;
}

BoneMask BoneMask::operator*(const BoneMask& other) const {
    BoneMask result = *this;
    return result *= other;
}

BoneMask BoneMask::operator|(const BoneMask& other) const {
    BoneMask result = *this;
    return result |= other;
}

BoneMask BoneMask::operator&(const BoneMask& other) const {
    BoneMask result = *this;
    return result &= other;
}

BoneMask& BoneMask::operator*=(float scalar) {
    for (size_t i = 0; i < m_weights.size(); i++) {
        setWeight(i, m_weights[i] * scalar);
    }
    return *this;
}

BoneMask BoneMask::operator*(float scalar) const {
    BoneMask result = *this;
    return result *= scalar;
}

BoneMask& BoneMask::operator+=(float scalar) {
    for (size_t i = 0; i < m_weights.size(); i++) {
        setWeight(i, m_weights[i] + scalar);
    }
    return *this;
}

BoneMask BoneMask::operator+(float scalar) const {
    BoneMask result = *this;
    return result += scalar;
}

BoneMask BoneMask::operator~() const {
    BoneMask result = *this;
    for (size_t i = 0; i < m_weights.size(); i++) {
        result.setWeight(i, 1.0f - m_weights[i]);
    }
    return result;
}
