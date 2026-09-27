#ifndef TOOMANYBLOCKS_AABBTREE_H
#define TOOMANYBLOCKS_AABBTREE_H

#include <functional>
#include <glm/glm.hpp>
#include <vector>

#include "engine/geometry/BoundingVolume.h"

static constexpr int INVALID_TREE_NODE = -1;

template <typename T>
class AABBTree {
private:
    struct Node {
        BoundingBox bounds;

        int parent = INVALID_TREE_NODE;
        int left = INVALID_TREE_NODE;
        int right = INVALID_TREE_NODE;

        // Used when this node is on the free list
        int nextFree = INVALID_TREE_NODE;

        T object;

        int height;

        // A leaf has no chilren, intermediates always have left + right children
        bool isLeaf() const { return left == INVALID_TREE_NODE; }
    };

    std::vector<Node> m_nodes;
    int m_root;

    int m_freeListRoot;
    size_t m_freeNodeCount;

    float m_fatMargin;

    int allocateNode() {
        // Reuse a node from the free list when possible
        if (m_freeListRoot != INVALID_TREE_NODE) {
            int nodeId = m_freeListRoot;

            m_freeListRoot = m_nodes[nodeId].nextFree;
            m_nodes[nodeId] = Node{};
            m_freeNodeCount--;

            return nodeId;
        }

        int nodeId = static_cast<int>(m_nodes.size());
        m_nodes.emplace_back();
        return nodeId;
    }

    void freeNode(int nodeId) {
        Node& node = m_nodes[nodeId];
        node.nextFree = m_freeListRoot;  // Point node to be freed at current free list end
        node.height = -1;

        // New free list root is the newly freed node
        m_freeListRoot = nodeId;
        m_freeNodeCount++;
    }

    void insertLeaf(int leafId) {
        // When tree empty, leaf becomes root
        if (m_root == INVALID_TREE_NODE) {
            m_root = leafId;
            m_nodes[leafId].parent = INVALID_TREE_NODE;
            return;
        }

        const BoundingBox leafBounds = m_nodes[leafId].bounds;

        // Walk down the tree to look for the sibling that would result in the smallest cost increase
        int siblingCandidateId = m_root;

        while (!m_nodes[siblingCandidateId].isLeaf()) {
            Node& currentNode = m_nodes[siblingCandidateId];

            int leftChildId = currentNode.left;
            int rightChildId = currentNode.right;

            const Node& leftChild = m_nodes[leftChildId];
            const Node& rightChild = m_nodes[rightChildId];

            float currentArea = currentNode.bounds.surfaceArea();

            BoundingBox combinedBounds = BoundingBox::combined(currentNode.bounds, leafBounds);
            float combinedArea = combinedBounds.surfaceArea();
            float inheritanceCost = 2.0f * (combinedArea - currentArea);

            // Calculate left side descending cost
            float leftCost;

            BoundingBox combinedLeftBounds = BoundingBox::combined(leftChild.bounds, leafBounds);
            if (leftChild.isLeaf()) {
                leftCost = combinedLeftBounds.surfaceArea() + inheritanceCost;
            } else {
                float oldLeftArea = leftChild.bounds.surfaceArea();
                float newLeftArea = combinedLeftBounds.surfaceArea();
                leftCost = (newLeftArea - oldLeftArea) + inheritanceCost;
            }

            // Calculate right side descending cost
            float rightCost;

            BoundingBox combinedRightBounds = BoundingBox::combined(rightChild.bounds, leafBounds);
            if (rightChild.isLeaf()) {
                rightCost = combinedRightBounds.surfaceArea() + inheritanceCost;
            } else {
                float oldRightArea = rightChild.bounds.surfaceArea();
                float newRightArea = combinedRightBounds.surfaceArea();
                rightCost = (newRightArea - oldRightArea) + inheritanceCost;
            }

            // Stop if making current node parent of subtree is cheaper than descending
            float createParentCost = 2.0f * combinedArea;
            if (createParentCost < leftCost && createParentCost < rightCost) {
                break;
            }

            // Continue with cheapest subtree
            siblingCandidateId = leftCost < rightCost ? leftChildId : rightChildId;
        }

        int oldParentId = m_nodes[siblingCandidateId].parent;

        int newParentId = allocateNode();
        Node& newParent = m_nodes[newParentId];
        newParent.parent = oldParentId;
        newParent.bounds = BoundingBox::combined(leafBounds, m_nodes[siblingCandidateId].bounds);

        newParent.height = m_nodes[siblingCandidateId].height + 1;

        newParent.left = siblingCandidateId;
        newParent.right = leafId;

        m_nodes[siblingCandidateId].parent = newParentId;
        m_nodes[leafId].parent = newParentId;

        // Connect new parent to old tree
        if (oldParentId == INVALID_TREE_NODE) {
            m_root = newParentId;
        } else {
            Node& oldParent = m_nodes[oldParentId];
            if (oldParent.left == siblingCandidateId) {
                oldParent.left = newParentId;
            } else {
                oldParent.right = newParentId;
            }
        }

        fixUpwards(m_nodes[leafId].parent);
    }

    void removeLeaf(int leafId) {
        if (leafId == m_root) {
            m_root = INVALID_TREE_NODE;
            return;
        }

        int parentId = m_nodes[leafId].parent;
        int grandParentId = m_nodes[parentId].parent;

        int siblingId;
        if (m_nodes[parentId].left == leafId) {
            siblingId = m_nodes[parentId].right;
        } else {
            siblingId = m_nodes[parentId].left;
        }

        if (grandParentId != INVALID_TREE_NODE) {
            // If parent not root, remove parent and connnect granparent and sibling directy
            Node& grandParent = m_nodes[grandParentId];

            if (grandParent.left == parentId)
                grandParent.left = siblingId;
            else
                grandParent.right = siblingId;

            m_nodes[siblingId].parent = grandParentId;

            freeNode(parentId);
            fixUpwards(grandParentId);
        } else {
            // Parent was root, sibling may simply become new root
            m_root = siblingId;
            m_nodes[siblingId].parent = INVALID_TREE_NODE;
            freeNode(parentId);
        }

        m_nodes[leafId].parent = INVALID_TREE_NODE;
    }

    int balance(int nodeId) {
        Node& node = m_nodes[nodeId];

        // Node with height below 2 cannot have subtree that is one level taller than the other
        if (node.height < 2) return nodeId;

        int leftChildId = node.left;
        int rightChildId = node.right;
        Node& leftChild = m_nodes[leftChildId];
        Node& rightChild = m_nodes[rightChildId];

        int balanceFactor = rightChild.height - leftChild.height;

        // Check if right subtree is too tall
        if (balanceFactor > 1) {
            int rightLeftChildId = rightChild.left;
            int rightRightChildId = rightChild.right;
            Node& rightLeftChild = m_nodes[rightLeftChildId];
            Node& rightRightChild = m_nodes[rightRightChildId];

            // Promote right child
            rightChild.left = nodeId;
            rightChild.parent = node.parent;

            node.parent = rightChildId;

            // Connect promoted node to origianl parent
            if (rightChild.parent != INVALID_TREE_NODE) {
                Node& parent = m_nodes[rightChild.parent];
                if (parent.left == nodeId) {
                    parent.left = rightChildId;
                } else {
                    parent.right = rightChildId;
                }
            } else {
                m_root = rightChildId;
            }

            // Keep the taller subtree closer to the top
            if (rightLeftChild.height > rightRightChild.height) {
                rightChild.right = rightLeftChildId;

                node.right = rightRightChildId;
                rightRightChild.parent = nodeId;
            } else {
                rightChild.right = rightRightChildId;

                node.right = rightLeftChildId;
                rightLeftChild.parent = nodeId;
            }

            // Recalculate
            node.bounds = BoundingBox::combined(m_nodes[node.left].bounds, m_nodes[node.right].bounds);
            rightChild.bounds = BoundingBox::combined(node.bounds, m_nodes[rightChild.right].bounds);
            node.height = 1 + std::max(m_nodes[node.left].height, m_nodes[node.right].height);
            rightChild.height = 1 + std::max(node.height, m_nodes[rightChild.right].height);
            return rightChildId;
        }

        // Check if the left subtree is too tall
        if (balanceFactor < -1) {
            int leftLeftChildId = leftChild.left;
            int leftRightChildId = leftChild.right;
            Node& leftLeftChild = m_nodes[leftLeftChildId];
            Node& leftRightChild = m_nodes[leftRightChildId];

            // Promote the left child
            leftChild.left = nodeId;
            leftChild.parent = node.parent;

            node.parent = leftChildId;

            // Connect promoted node to origianl parent
            if (leftChild.parent != INVALID_TREE_NODE) {
                Node& parent = m_nodes[leftChild.parent];
                if (parent.left == nodeId) {
                    parent.left = leftChildId;
                } else {
                    parent.right = leftChildId;
                }
            } else {
                m_root = leftChildId;
            }

            // Keep the taller subtree closer to the top
            if (leftLeftChild.height > leftRightChild.height) {
                leftChild.right = leftLeftChildId;

                node.left = leftRightChildId;
                leftRightChild.parent = nodeId;
            } else {
                leftChild.right = leftRightChildId;

                node.left = leftLeftChildId;
                leftLeftChild.parent = nodeId;
            }

            // Recalculate
            node.bounds = BoundingBox::combined(m_nodes[node.left].bounds, m_nodes[node.right].bounds);
            leftChild.bounds = BoundingBox::combined(node.bounds, m_nodes[leftChild.right].bounds);
            node.height = 1 + std::max(m_nodes[node.left].height, m_nodes[node.right].height);
            leftChild.height = 1 + std::max(node.height, m_nodes[leftChild.right].height);
            return leftChildId;
        }

        // Good balance
        return nodeId;
    }

    void fixUpwards(int nodeId) {
        int currentNodeId = nodeId;
        // Walk back to root, and balance / rotate if nessecary + fix bounds
        while (currentNodeId != INVALID_TREE_NODE) {
            currentNodeId = balance(currentNodeId);
            Node& node = m_nodes[currentNodeId];

            int leftChildId = node.left;
            int rightChildId = node.right;
            node.height = 1 + std::max(m_nodes[leftChildId].height, m_nodes[rightChildId].height);
            node.bounds = BoundingBox::combined(m_nodes[leftChildId].bounds, m_nodes[rightChildId].bounds);
            currentNodeId = node.parent;
        }
    }

public:
    AABBTree(float fatMargin = 1.0f)
        : m_root(INVALID_TREE_NODE), m_freeListRoot(INVALID_TREE_NODE), m_freeNodeCount(0), m_fatMargin(fatMargin) {}

    int insert(T object, const BoundingBox& bounds) {
        int leafId = allocateNode();

        Node& leaf = m_nodes[leafId];

        // Insert fat AABB to avoid reinsertion on small movement updates on nodes
        leaf.bounds = bounds.extendedBy(m_fatMargin);
        leaf.object = object;
        leaf.height = 0;

        insertLeaf(leafId);

        return leafId;
    }

    void remove(int nodeId) {
        if (!isValid(nodeId)) return;

        removeLeaf(nodeId);
        freeNode(nodeId);
    }

    bool update(int nodeId, const BoundingBox& newBounds) {
        if (!isValid(nodeId)) return false;

        Node& leaf = m_nodes[nodeId];

        // Early return if still contained in fat ABBB
        if (leaf.bounds.contains(newBounds)) return false;

        // Reinsert
        removeLeaf(nodeId);
        leaf.bounds = newBounds.extendedBy(m_fatMargin);
        insertLeaf(nodeId);
        return true;
    }

    void query(const BoundingBox& queryBounds, std::function<void(T)> callback) const {
        if (m_root == INVALID_TREE_NODE) return;

        std::vector<int> stack;
        stack.reserve(128);
        stack.push_back(m_root);

        while (!stack.empty()) {
            const Node& node = m_nodes[stack.back()];
            stack.pop_back();

            // Entire subtree is outside the query.
            if (!node.bounds.overlaps(queryBounds)) continue;

            if (node.isLeaf()) {
                callback(node.object);
                continue;
            }

            stack.push_back(node.left);
            stack.push_back(node.right);
        }
    }

    bool isValid(int nodeId) const {
        if (nodeId == INVALID_TREE_NODE) return false;
        if (nodeId >= m_nodes.size()) return false;

        // A height of -1 means the node is currently on the free list
        return m_nodes[nodeId].height >= 0;
    }

    const BoundingBox& getBounds(int nodeId) const { return m_nodes[nodeId].bounds; }

    size_t getNodeCount() const { return m_nodes.size() - m_freeNodeCount; }

    size_t getCapacity() const { return m_nodes.size(); }

    bool empty() const { return m_root == INVALID_TREE_NODE; }
};

#endif