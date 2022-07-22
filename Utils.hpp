#pragma once

#include <glm/glm.hpp>

inline float repeat(float x, float min, float max)
{
    float normalized = x;
    while (normalized > max) {
        normalized -= 2 * max;
    }
    while (normalized < min) {
        normalized -= 2 * min;
    }
    return normalized;
}

inline void normalize(glm::vec3& v, float min, float max)
{
    v.x = repeat(v.x, min, max);
    v.y = repeat(v.y, min, max);
    v.z = repeat(v.z, min, max);
}

inline void normalize(glm::vec3& v)
{
    // default bounds within -pi : pi
    normalize(v, -M_PI, M_PI);
}
