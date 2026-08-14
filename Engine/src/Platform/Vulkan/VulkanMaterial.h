#pragma once

#include "ZEngine/Renderer/Material.h"
#include "Platform/Vulkan/VulkanBuffer.h"

#include "Platform/Vulkan/VulkanDescriptorAllocator.h"

#include <vulkan/vulkan_raii.hpp>

namespace ZEngine {

	class VulkanMaterial : public Material {
	public:
		VulkanMaterial(const std::string& name, const Ref<Texture2D>& texture);
        ~VulkanMaterial() = default;

        void Init(const Scope<DescriptorAllocator>& descriptorAllocator, const Scope<LayoutManager>& layoutManager) override;

        void SetAlbedoColor(const glm::vec4& color) override;
        const glm::vec4& GetAlbedoColor() const override { return m_Properties.Albedo; }

        void SetRoughness(float roughness) override;
        float GetRoughness() const override { return m_Properties.Roughness; }

        void SetMetallic(float metallic) override;
        float GetMetallic() const override { return m_Properties.Metallic; }

        void SetAlbedoTexture(Ref<Texture2D> texture) override;
        Ref<Texture2D> GetAlbedoTexture() const override { return m_AlbedoTexture; }

        const vk::raii::DescriptorSet& GetDescriptorSet() { return m_MaterialSet; }

        void UpdateBuffer();

    private:
        void AllocateDescriptorSet(const Scope<DescriptorAllocator>& descriptorAllocator, const Scope<LayoutManager>& layoutManager);
        void UpdateDescriptorSet();

    private:
        std::string m_Name;
        MaterialProperties m_Properties {};

        Ref<Texture2D> m_AlbedoTexture;
        Ref<VulkanUniformBuffer> m_MaterialUBO;

        vk::raii::DescriptorSet m_MaterialSet = nullptr;
        bool m_IsDirty = true;
	};

}