#ifndef TOOMANYBLOCKS_UIUTIL_H
#define TOOMANYBLOCKS_UIUTIL_H

#include <glm/vec3.hpp>

#include "engine/rendering/passes/debug/DebugReport.h"

namespace UI::Util {
    void CenterPopup();
    void MakeNextWindowFullscreen();

    void DrawCrosshair(float crossSize, float thickness);

    void DrawCoordinateSystem(
        glm::vec3 x,
        glm::vec3 y,
        glm::vec3 z,
        float canvasSize,
        float axisLength,
        const char* xLabel = "X",
        const char* yLabel = "Y",
        const char* zLabel = "Z"
    );

    void DrawDebugNode(const DebugNode& node);
}  // namespace UI::Util

#endif
