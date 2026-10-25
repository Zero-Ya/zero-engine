#pragma once

#include "Frustum.h"

namespace ZEngine {

    bool Plane::IsOnOrForwardPlane(const AABB& aabb) const {
        // P-vertex: project the AABB min/max along the plane normal
        // If the normal component is positive, pick max; otherwise pick min.
        glm::vec3 pVertex = {
            normal.x >= 0.0f ? aabb.max.x : aabb.min.x,
            normal.y >= 0.0f ? aabb.max.y : aabb.min.y,
            normal.z >= 0.0f ? aabb.max.z : aabb.min.z
        };

        // If the p-vertex is behind the plane (negative distance), the whole box is outside
        return GetSignedDistance(pVertex) >= 0.0f;
    }

    bool Frustum::IsOnFrustum(const AABB& aabb) const {
        // Check against all 6 planes. Order by importance for faster early-exit short-circuiting
        return
            leftFace.IsOnOrForwardPlane(aabb) &&
            rightFace.IsOnOrForwardPlane(aabb) &&
            bottomFace.IsOnOrForwardPlane(aabb) &&
            topFace.IsOnOrForwardPlane(aabb) &&
            nearFace.IsOnOrForwardPlane(aabb) &&
            farFace.IsOnOrForwardPlane(aabb);
    }

    Frustum Frustum::ExtractFrustumFromMatrix(const glm::mat4& vp) {
        Frustum frustum;
        glm::mat4 m = glm::transpose(vp);

        // Left
        frustum.leftFace.normal = glm::vec3(m[3] + m[0]);
        frustum.leftFace.distance = m[3].w + m[0].w;

        // Right
        frustum.rightFace.normal = glm::vec3(m[3] - m[0]);
        frustum.rightFace.distance = m[3].w - m[0].w;

        // Bottom
        frustum.bottomFace.normal = glm::vec3(m[3] + m[1]);
        frustum.bottomFace.distance = m[3].w + m[1].w;

        // Top
        frustum.topFace.normal = glm::vec3(m[3] - m[1]);
        frustum.topFace.distance = m[3].w - m[1].w;

        // Near
        frustum.nearFace.normal = glm::vec3(m[3] + m[2]);
        frustum.nearFace.distance = m[3].w + m[2].w;

        // Far
        frustum.farFace.normal = glm::vec3(m[3] - m[2]);
        frustum.farFace.distance = m[3].w - m[2].w;

        // Normalize all planes
        auto normalizePlane = [](Plane& p) {
            float length = glm::length(p.normal);
            p.normal /= length;
            p.distance /= length;
            };

        normalizePlane(frustum.leftFace);
        normalizePlane(frustum.rightFace);
        normalizePlane(frustum.bottomFace);
        normalizePlane(frustum.topFace);
        normalizePlane(frustum.nearFace);
        normalizePlane(frustum.farFace);

        return frustum;
    }

}