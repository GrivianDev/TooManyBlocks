#ifndef TOOMANYBLOCKS_DEBUGREPORT_H
#define TOOMANYBLOCKS_DEBUGREPORT_H

#include <vector>

enum class DebugValueType {
    Int,
    Float,
    TimeMs,
};

struct DebugValue {
    const char* name;

    DebugValueType type;

    union {
        int intValue;
        float floatValue;
    };
};

struct DebugNode {
    const char* name;
    std::vector<DebugValue> values;
    std::vector<DebugNode> children;
};

class DebugReport {
private:
    DebugNode root;
    std::vector<DebugNode*> nodeStack;

    DebugNode* currentNode();

public:
    DebugReport();

    void clear();

    void addCounter(const char* name, int value);

    void addFloat(const char* name, float value);

    void addTimeMs(const char* name, float value);

    void beginGroup(const char* name);

    void endGroup();

    inline const DebugNode& getRoot() const { return root; }
};

#endif
