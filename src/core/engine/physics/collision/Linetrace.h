#ifndef TOOMANYBLOCKS_LINETRACE_H
#define TOOMANYBLOCKS_LINETRACE_H

#include <glm/glm.hpp>

struct HitResult {
    bool hitSuccess;
    glm::vec3 objectPosition;
    glm::vec3 impactPoint;
};

enum Channel {
    BlockTrace
};

HitResult linetraceByChannel(const glm::vec3& start, const glm::vec3& end, Channel channel);

#endif