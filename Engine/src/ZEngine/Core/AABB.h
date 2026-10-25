#pragma once

#include <glm/glm.hpp>

namespace ZEngine {

    struct AABB {
        glm::vec3 min = { 0.0f, 0.0f, 0.0f };
        glm::vec3 max = { 0.0f, 0.0f, 0.0f };

        // Transforms a local AABB into world space given a model matrix
        AABB Transform(const glm::mat4& modelMatrix) const {
            glm::vec3 center = (min + max) * 0.5f;
            glm::vec3 extent = (max - min) * 0.5f;

            // Transform center
            glm::vec3 newCenter = glm::vec3(modelMatrix * glm::vec4(center, 1.0f));

            // Compute rotated/scaled extents
            glm::mat3 absModel = glm::mat3(
                std::abs(modelMatrix[0][0]), std::abs(modelMatrix[0][1]), std::abs(modelMatrix[0][2]),
                std::abs(modelMatrix[1][0]), std::abs(modelMatrix[1][1]), std::abs(modelMatrix[1][2]),
                std::abs(modelMatrix[2][0]), std::abs(modelMatrix[2][1]), std::abs(modelMatrix[2][2])
            );
            glm::vec3 newExtent = absModel * extent;

            return AABB{ newCenter - newExtent, newCenter + newExtent };
        }
    };

}