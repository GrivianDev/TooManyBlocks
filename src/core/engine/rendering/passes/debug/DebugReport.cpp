#include "DebugReport.h"

#include <stdexcept>

DebugNode* DebugReport::currentNode() {
    if (nodeStack.empty()) {
        throw std::runtime_error("node stack empty");
    }

    !nodeStack.empty();
    return nodeStack.back();
}

DebugReport::DebugReport() {
    root.name = "Root";
    nodeStack.push_back(&root);
}

void DebugReport::clear() {
    root.values.clear();
    root.children.clear();
    nodeStack.clear();
    nodeStack.push_back(&root);
}

void DebugReport::addCounter(const char* name, int value) {
    DebugValue v;
    v.name = name;
    v.type = DebugValueType::Int;
    v.intValue = value;

    currentNode()->values.push_back(v);
}

void DebugReport::addFloat(const char* name, float value) {
    DebugValue v;
    v.name = name;
    v.type = DebugValueType::Float;
    v.floatValue = value;

    currentNode()->values.push_back(v);
}

void DebugReport::addTimeMs(const char* name, float value) {
    DebugValue v;
    v.name = name;
    v.type = DebugValueType::TimeMs;
    v.floatValue = value;

    currentNode()->values.push_back(v);
}

void DebugReport::beginGroup(const char* name) {
    DebugNode& child = currentNode()->children.emplace_back();
    child.name = name;

    nodeStack.push_back(&child);
}

void DebugReport::endGroup() {
    // Prevent popping the root
    if (nodeStack.size() <= 1) throw std::runtime_error("DebugReport::endGroup() called without matching beginGroup()");
    if (nodeStack.size() > 1) nodeStack.pop_back();
}
