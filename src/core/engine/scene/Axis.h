#ifndef TOOMANYBLOCKS_AXIS_H
#define TOOMANYBLOCKS_AXIS_H

#include <cstdint>
#include <glm/vec3.hpp>

constexpr glm::vec3 WorldUp = glm::vec3(0, 1, 0);
constexpr glm::vec3 WorldForward = glm::vec3(0, 0, -1);
constexpr glm::vec3 WorldRight = glm::vec3(1, 0, 0);

enum Axis {
    X = 0,
    Y,
    Z,
};

enum class AxisDirection : uint8_t {
    PositiveX = 0u,
    NegativeX,
    PositiveY,
    NegativeY,
    PositiveZ,
    NegativeZ,
};

constexpr Axis allAxis[3] = {
    Axis::X,
    Axis::Y,
    Axis::Z,
};

constexpr AxisDirection allAxisDirections[6] = {
    AxisDirection::PositiveX, AxisDirection::NegativeX, AxisDirection::PositiveY, AxisDirection::NegativeY, AxisDirection::PositiveZ, AxisDirection::NegativeZ,
};

#endif
