#pragma once

#include "AABB.h"

#include <glm/glm.hpp>

namespace ZEngine {

    struct Plane {
        glm::vec3 normal = { 0.0f, 1.0f, 0.0f };
        float distance = 0.0f; // Distance from origin

        // Returns signed distance from a point to the plane
        float GetSignedDistance(const glm::vec3& point) const {
            return glm::dot(normal, point) + distance;
        }

        bool IsOnOrForwardPlane(const AABB& aabb) const;
    };

    struct Frustum {
        Plane leftFace;
        Plane rightFace;
        Plane bottomFace;
        Plane topFace;
        Plane nearFace;
        Plane farFace;

        bool IsOnFrustum(const AABB& aabb) const;

        static Frustum ExtractFrustumFromMatrix(const glm::mat4& vp);
    };

}