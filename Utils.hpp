#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>

#include <iostream>

class BBox {
public:
    BBox() = default;
    BBox(const glm::vec3& minIn, const glm::vec3& maxIn)
    {
        // initial bounds (relative to origin and no rotation)
        min0 = minIn;
        max0 = maxIn;
        midpt = get_midpoint0();
        extent = max0 - min0;
    }

    bool contains_pt(const glm::vec3& pt) const
    {
        // first rotate pt by the origin of this bbox by -yaw
        glm::vec3 pt_midpt = pt - midpt; // vector from origin of this box to the pt

        // rotate pt_midpt
        float yaw = rot.z; // since this bbox can be alined along yaw only
        glm::mat4 rotMat(1);
        // rotate point by -yaw to reach axis aligned
        rotMat = glm::rotate(rotMat, -yaw, glm::vec3(0, 0, 1.f));
        glm::vec3 AA_pt = glm::vec3(rotMat * glm::vec4(pt_midpt, 1.0));

        // now that the pt is axis-aligned to the original bounds, the check is trivial
        bool within_x = (AA_pt.x >= min0.x && AA_pt.x <= max0.x);
        bool within_y = (AA_pt.y >= min0.y && AA_pt.y <= max0.y);
        bool within_z = (AA_pt.z >= min0.z && AA_pt.z <= max0.z);
        return within_x && within_y && within_z;
    }

    bool collides_with(const BBox& other) const
    {
        return false;
    }

    void update(const glm::vec3& pos, const float yaw)
    {
        /// NOTE: for now these bboxes only support rotation along yaw
        rot = glm::vec3(0, 0, yaw);

        // translate to match pos
        midpt = pos + get_midpoint0();
    }

    glm::vec3 get_midpoint0() const
    {
        return (max0 + min0) / 2.f;
    }
    glm::mat3 get_rotation_mat() const
    {
        float yaw = rot.z;
        return glm::mat3(
            glm::vec3(glm::cos(yaw), -glm::sin(yaw), 0),
            glm::vec3(glm::sin(yaw), glm::cos(yaw), 0),
            glm::vec3(0, 0, 1));
    }
    glm::mat4x3 get_mat() const
    {
        glm::mat4 rot4 = glm::toMat4(glm::quat(rot));
        glm::mat4 trans = glm::translate(glm::mat4(1.0f), midpt);
        glm::mat4 scale = glm::mat4(1.0);
        scale[0][0] = extent.x / 2.f;
        scale[1][1] = extent.y / 2.f;
        scale[2][2] = extent.z / 2.f;
        // first scale, then rotate, then transform
        return trans * rot4 * scale;
    }

    glm::vec3 min0, max0;

    glm::vec3 midpt;
    glm::vec3 extent;
    glm::vec3 rot;
};

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
