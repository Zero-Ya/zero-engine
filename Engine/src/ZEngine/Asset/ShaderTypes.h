#pragma once

#include "ZEngine/RHI/RHICommon.h"

#include <glm/glm.hpp>

namespace ZEngine {

    struct DirectionalLight {
        glm::vec3 direction { 0.0f, -1.0f ,0.0f };
        float intensity = 1.0f;

        glm::vec3 color { 1.0f, 1.0f, 1.0f };
        float padding = 0.0f;
    };

    struct CameraData {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };

    struct SceneUniformData {
        CameraData camera;
        DirectionalLight sunLight;
        glm::vec4 cameraPos;

        glm::mat4 lightSpaceMatrix;
    };

    struct ShadowMapData {
        uint32_t textureIndex;
        uint32_t samplerIndex;
    };

    struct ShadowPushConstants {
        glm::mat4 lightSpaceMatrix;
        glm::mat4 modelMatrix;
    };

    struct CubemapData {
        uint32_t textureIndex;
        uint32_t samplerIndex;
    };

    struct MeshPushConstants {
        glm::mat4 modelMatrix;
        uint32_t albedoTextureIndex;
        uint32_t normalTextureIndex;
        uint32_t roughnessTextureIndex;
        uint32_t samplerIndex;
    };

    struct PushConstantData {
        MeshPushConstants meshConstants;
        ShadowMapData shadowMapData;
    };

    struct MaterialData {
        glm::vec4 baseColorFactor { 1.0f, 1.0f, 1.0f, 1.0f };
        glm::vec3 emissiveFactor { 0.0f, 0.0f, 0.0f };
        float metallicFactor = 1.0f;
        float roughnessFactor = 1.0f;
        float alphaCutoff = 0.5f;

        // Bindless texture indices
        uint32_t albedoTextureIndex = 0;
        uint32_t normalTextureIndex = 0;
        uint32_t metallicRoughnessTextureIndex = 0;
        uint32_t emissiveTextureIndex = 0;
        uint32_t samplerIndex = 0;
        uint32_t flags = 0;

        uint32_t padding[2] { 0, 0 };
    };

}