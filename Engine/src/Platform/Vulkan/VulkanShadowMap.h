#pragma once

#include "ZEngine/Renderer/ShadowMap.h"

#include "ZEngine/RHI/RHICommon.h"

#include <vulkan/vulkan_raii.hpp>

namespace ZEngine {

    class VulkanGraphicsDevice;
    class VulkanResourceManager;
    class VulkanBindlessManager;

    class VulkanShadowMap : public ShadowMap {
    public:
        VulkanShadowMap(const Scope<GraphicsDevice>& graphicsDevice, const Scope<BindlessDescriptorManager>& bindlessManager, const Scope<ResourceManager>& resourceManager);
        ~VulkanShadowMap() = default;

        TextureHandle Init(const PipelineSpecification& spec, uint32_t shadowResolution = 2048) override;

        void CreatePipelineState(const PipelineSpecification& spec);

        const vk::raii::Pipeline& GetPipeline() const { return m_Pipeline; }
        vk::PipelineLayout GetPipelineLayout() const { return *m_PipelineLayout; }

        const uint32_t GetTextureIndex() const { return m_TextureIndex; }
        const uint32_t GetSamplerIndex() const { return m_SamplerIndex; }

    private:
        uint32_t RegisterTexture(TextureHandle handle, const TextureLayout& layout);
        uint32_t RegisterSampler();

        vk::Format ShaderDataTypeToVulkanFormat(ShaderDataType type) {
            switch (type) {
            case ShaderDataType::Float:  return vk::Format::eR32Sfloat;
            case ShaderDataType::Float2: return vk::Format::eR32G32Sfloat;
            case ShaderDataType::Float3: return vk::Format::eR32G32B32Sfloat;
            case ShaderDataType::Float4: return vk::Format::eR32G32B32A32Sfloat;
            default: ZE_CORE_ASSERT(false, "Unknown ShaderDataType passed to Vulkan pipeline layout compiler!");
            }
            return vk::Format::eUndefined;
        }

    private:
        vk::raii::Pipeline m_Pipeline = nullptr;
        vk::raii::PipelineLayout m_PipelineLayout = nullptr;

        uint32_t m_TextureIndex;
        uint32_t m_SamplerIndex;

        VulkanGraphicsDevice* m_GraphicsDevice = nullptr;
        VulkanResourceManager* m_ResourceManager = nullptr;
        VulkanBindlessManager* m_BindlessManager = nullptr;
    };

}