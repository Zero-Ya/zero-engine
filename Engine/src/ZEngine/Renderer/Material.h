#pragma once

#include "ZEngine/Renderer/Texture.h"

#include <glm/glm.hpp>

namespace ZEngine {

    struct MaterialProperties {
        glm::vec4 Albedo { 1.0f, 1.0f, 1.0f, 1.0f };
        float Roughness = 0.5f;
        float Metallic = 0.0f;
        glm::vec2 Padding { 0.0f, 0.0f };
    };

    class Material {
    public:
        virtual ~Material() = default;

        virtual void Init() = 0;

        virtual void SetAlbedoColor(const glm::vec4& color) = 0;
        virtual const glm::vec4& GetAlbedoColor() const = 0;

        virtual void SetRoughness(float roughness) = 0;
        virtual float GetRoughness() const = 0;

        virtual void SetMetallic(float metallic) = 0;
        virtual float GetMetallic() const = 0;

        virtual void SetAlbedoTexture(Ref<Texture2D> texture) = 0;
        virtual Ref<Texture2D> GetAlbedoTexture() const = 0;

        static Ref<Material> Create(const std::string& name, const Ref<Texture2D>& texture);
    };

}