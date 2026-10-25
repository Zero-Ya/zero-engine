#pragma once

#include "ZEngine/Renderer/BindlessDescriptorManager.h"

#include <vulkan/vulkan_raii.hpp>

namespace ZEngine {

    class VulkanGraphicsDevice;
    class VulkanResourceManager;

	class VulkanBindlessManager : public BindlessDescriptorManager {
    public:
        VulkanBindlessManager(const Scope<GraphicsDevice>& graphicsDevice);
        ~VulkanBindlessManager() = default;

        uint32_t RegisterTexture(TextureHandle handle, const TextureLayout& layout);
        void UnregisterTexture(uint32_t bindlessIndex);

        uint32_t RegisterSampler(SamplerHandle handle);
        uint32_t RegisterCmpSampler(SamplerHandle handle);
        void UnregisterSampler(uint32_t bindlessIndex);

        void UpdateSceneBuffer(vk::Buffer sceneBuffer, vk::DeviceSize size);

        void SetResourceManager(const Scope<ResourceManager>& resourceManager) override;

        vk::DescriptorSetLayout GetSet0Layout() const { return m_Set0Layout; }
        vk::DescriptorSetLayout GetSet1Layout() const { return m_Set0Layout; }
        vk::DescriptorSet GetDescriptorSet() const { return *m_DescriptorSet; }
        vk::PipelineLayout GetPipelineLayout() const { return *m_PipelineLayout; }

        std::vector<vk::raii::DescriptorSet> AllocateSceneSet();

    private:
        void CreateLayoutAndPool();
        void CreatePipelineLayout();

        vk::ImageLayout ConvertLayout(TextureLayout layout) const;

        VulkanGraphicsDevice* m_GraphicsDevice;
        VulkanResourceManager* m_ResourceManager;

        vk::raii::DescriptorSetLayout m_Set0Layout = nullptr;
        vk::raii::DescriptorSetLayout m_Set1Layout = nullptr;

        vk::raii::DescriptorPool m_DescriptorPool = nullptr;
        vk::raii::DescriptorSet m_DescriptorSet   = nullptr;
        vk::raii::PipelineLayout m_PipelineLayout = nullptr;
	};

}