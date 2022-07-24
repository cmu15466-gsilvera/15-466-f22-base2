#pragma once

#include <glm/glm.hpp>
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
    }

    void Print() const
    {
        std::cout << "BBox: " << glm::to_string(min) << " -> " << glm::to_string(max) << std::endl;
    }

    bool contains_pt(const glm::vec3& pt) const
    {
        // first rotate pt by the origin of this bbox by -yaw
        glm::vec3 midpt = get_midpoint();
        glm::vec3 pt_midpt = pt - midpt; // vector from origin of this box to the pt

        // rotate pt_midpt
        float yaw = rot.z; // since this bbox can be alined along yaw only
        glm::mat4 rotMat(1);
        rotMat = glm::rotate(rotMat, -yaw, glm::vec3(0, 0, 1.f));
        glm::vec3 AA_pt = glm::vec3(rotMat * glm::vec4(pt_midpt, 1.0));

        // now that the pt is axis-aligned to the original bounds, the check is trivial
        bool within_x = (AA_pt.x >= min0.x && AA_pt.x <= max0.x);
        bool within_y = (AA_pt.y >= min0.y && AA_pt.y <= max0.y);
        bool within_z = (AA_pt.z >= min0.z && AA_pt.z <= max0.z);
        return within_x && within_y && within_z;
    }

    bool collides_with(const BBox &other) const {
        glm::vec3 scale = get_scale();
        return ()
    }

    void update(const glm::vec3& pos, const float yaw)
    {
        /// NOTE: for now these bboxes only support rotation along yaw
        rot = glm::vec3(0, 0, yaw);
        glm::mat4 rotMat(1);
        rotMat = glm::rotate(rotMat, yaw, glm::vec3(0, 0, 1.f));

        // assign min/max to the initial values (relative to origin)
        min = min0;
        max = max0;

        // apply rotation around origin
        min = glm::vec3(rotMat * glm::vec4(min, 1.0));
        max = glm::vec3(rotMat * glm::vec4(max, 1.0));

        // translate to match pos
        min += pos;
        max += pos;
    }

    // get box side lengths of initial dimens
    glm::vec3 get_scale0() const
    {
        return glm::vec3(std::fabs(max0.x - min0.x), std::fabs(max0.y - min0.y), std::fabs(max0.z - min0.z));
    }
    // get box side lengths of transformed box
    glm::vec3 get_scale() const
    {
        return glm::vec3(std::fabs(max.x - min.x), std::fabs(max.y - min.y), std::fabs(max.z - min.z));
    }
    glm::vec3 get_midpoint() const
    {
        return (max + min) / 2.f;
    }
    glm::mat3 get_rotation() const
    {
        float yaw = rot.z;
        return glm::mat3(
            glm::vec3(glm::cos(yaw), -glm::sin(yaw), 0),
            glm::vec3(glm::sin(yaw), glm::cos(yaw), 0),
            glm::vec3(0, 0, 1));
    }
    glm::mat4x3 get_mat() const
    {
        glm::vec3 s = get_scale0() / 2.f;
        glm::vec3 pos = get_midpoint();
        glm::mat3 rotmat = get_rotation();

        return glm::mat4x3(
            glm::vec3(s.x * rotmat[0][0], rotmat[1][0], rotmat[2][0]),
            glm::vec3(rotmat[0][1], s.y * rotmat[1][1], rotmat[2][1]),
            glm::vec3(rotmat[0][2], rotmat[1][2], s.z * rotmat[2][2]),
            pos);
    }

    glm::vec3 min0, max0;
    glm::vec3 min, max;
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
