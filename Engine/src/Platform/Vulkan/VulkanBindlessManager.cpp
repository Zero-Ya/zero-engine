#include "VulkanBindlessManager.h"

#include "ZEngine/Asset/ShaderTypes.h"

#include "VulkanGraphicsDevice.h"
#include "VulkanContext.h"

#include "VulkanResourceManager.h"

namespace ZEngine {

    Scope<BindlessDescriptorManager> BindlessDescriptorManager::Create(const Scope<GraphicsDevice>& graphicsDevice) {
        return std::make_unique<VulkanBindlessManager>(graphicsDevice);
    }

    VulkanBindlessManager::VulkanBindlessManager(const Scope<GraphicsDevice>& graphicsDevice) {
        m_GraphicsDevice = static_cast<VulkanGraphicsDevice*>(graphicsDevice.get());

        CreateLayoutAndPool();
        CreatePipelineLayout();
    }

    void VulkanBindlessManager::CreateLayoutAndPool() {
        auto& context = m_GraphicsDevice->GetContext();
        auto& device = context.GetDevice();

        // First descriptor sets (Set 0)
        std::array<vk::DescriptorSetLayoutBinding, 4> bindingsSet0 {};

        // Binding 0: Texture array
        bindingsSet0[0].binding = 0;
        bindingsSet0[0].descriptorType = vk::DescriptorType::eSampledImage;
        bindingsSet0[0].descriptorCount = MAX_BINDLESS_TEXTURES;
        bindingsSet0[0].stageFlags = vk::ShaderStageFlagBits::eFragment;

        // Binding 1: Sampler array
        bindingsSet0[1].binding = 1;
        bindingsSet0[1].descriptorType = vk::DescriptorType::eSampler;
        bindingsSet0[1].descriptorCount = MAX_BINDLESS_SAMPLERS;
        bindingsSet0[1].stageFlags = vk::ShaderStageFlagBits::eFragment;

        // Binding 2: Material storage buffer (SSBO)
        bindingsSet0[2].binding = 2;
        bindingsSet0[2].descriptorType = vk::DescriptorType::eStorageBuffer;
        bindingsSet0[2].descriptorCount = 1;
        bindingsSet0[2].stageFlags = vk::ShaderStageFlagBits::eFragment;

        // Binding 3: Comparison sampler array
        bindingsSet0[3].binding = 3;
        bindingsSet0[3].descriptorType = vk::DescriptorType::eSampler;
        bindingsSet0[3].descriptorCount = MAX_COMPARISON_SAMPLERS;
        bindingsSet0[3].stageFlags = vk::ShaderStageFlagBits::eFragment;

        // Enable Bindless flags for Texture/Sampler array bindings (0 and 1)
        std::array<vk::DescriptorBindingFlags, 4> bindingFlags = {
            vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eUpdateAfterBind,
            vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eUpdateAfterBind,
            {},
            {}
        };

        // Create 1st descriptor set layout
        vk::DescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo { .bindingCount = bindingFlags.size(), .pBindingFlags = bindingFlags.data() };
        vk::DescriptorSetLayoutCreateInfo layoutInfo0 { 
            .pNext = &bindingFlagsInfo,
            .flags = vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool,
            .bindingCount = 4,
            .pBindings = bindingsSet0.data() };
        m_Set0Layout = vk::raii::DescriptorSetLayout(device, layoutInfo0);

        

        // Second descriptor sets (Set 1)
        vk::DescriptorSetLayoutBinding bindingsSet1 {
            .binding = 0,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment
        };

        // Create 2nd descriptor set layout
        vk::DescriptorSetLayoutCreateInfo layoutInfo1{
            .pNext = nullptr,
            .flags = {},
            .bindingCount = 1,
            .pBindings = &bindingsSet1 };
        m_Set1Layout = vk::raii::DescriptorSetLayout(device, layoutInfo1);

        // Create descriptor pool
        std::array<vk::DescriptorPoolSize, 4> poolSizes = {
            vk::DescriptorPoolSize{ vk::DescriptorType::eSampledImage, MAX_BINDLESS_TEXTURES },
            vk::DescriptorPoolSize{ vk::DescriptorType::eSampler, MAX_BINDLESS_SAMPLERS + MAX_COMPARISON_SAMPLERS },
            vk::DescriptorPoolSize{ vk::DescriptorType::eStorageBuffer, MAX_BINDLESS_STORAGES },
            vk::DescriptorPoolSize{ vk::DescriptorType::eUniformBuffer, MAX_BINDLESS_UNIFORMS },
        };

        vk::DescriptorPoolCreateInfo poolInfo {
            .flags = vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind | vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            .maxSets = context.MAX_FRAMES_IN_FLIGHT * 2,
            .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
            .pPoolSizes = poolSizes.data()
        };
        m_DescriptorPool = vk::raii::DescriptorPool(device, poolInfo);

        // Allocate set 0
        vk::DescriptorSetAllocateInfo alloc0Info { .descriptorPool = m_DescriptorPool,
                                                   .descriptorSetCount = 1,
                                                   .pSetLayouts = &*m_Set0Layout };
        auto sets0 = device.allocateDescriptorSets(alloc0Info);

        m_DescriptorSet = std::move(sets0[0]);
    }

    std::vector<vk::raii::DescriptorSet> VulkanBindlessManager::AllocateSceneSet() {
        auto& context = m_GraphicsDevice->GetContext();
        auto& device = context.GetDevice();

        // Allocate set 1
        std::vector<vk::DescriptorSetLayout> layouts(context.MAX_FRAMES_IN_FLIGHT, *m_Set1Layout);
        vk::DescriptorSetAllocateInfo alloc1Info { .descriptorPool = m_DescriptorPool,
                                                   .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
                                                   .pSetLayouts = layouts.data() };
        return device.allocateDescriptorSets(alloc1Info);
    }

    void VulkanBindlessManager::CreatePipelineLayout() {
        auto& context = m_GraphicsDevice->GetContext();
        auto& device = context.GetDevice();

        // Push constant range: 64 Bytes for mesh matrix
        vk::PushConstantRange pushConstantRange(
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
            0,
            sizeof(PushConstantData)
        );

        vk::DescriptorSetLayout layouts[] = {m_Set0Layout, m_Set1Layout};

        vk::PipelineLayoutCreateInfo layoutInfo {
            .setLayoutCount = 2,
            .pSetLayouts = layouts,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &pushConstantRange
        };
        m_PipelineLayout = vk::raii::PipelineLayout(device, layoutInfo);
    }

    uint32_t VulkanBindlessManager::RegisterTexture(TextureHandle handle, const TextureLayout& layout) {
        auto& context = m_GraphicsDevice->GetContext();
        auto& device = context.GetDevice();

        auto resource = m_ResourceManager->ResolveTexture(handle);

        vk::DescriptorImageInfo imageInfo { .imageView = resource->imageView, .imageLayout = ConvertLayout(layout)};
        vk::WriteDescriptorSet descriptorWrite { .dstSet = *m_DescriptorSet,
                                                 .dstBinding = 0, // Binding 0
                                                 .dstArrayElement = handle.GetIndex(),
                                                 .descriptorCount = 1,
                                                 .descriptorType = vk::DescriptorType::eSampledImage,
                                                 .pImageInfo = &imageInfo };
        device.updateDescriptorSets(descriptorWrite, nullptr);

        return handle.GetIndex();
    }

    void VulkanBindlessManager::UnregisterTexture(uint32_t bindlessIndex) {

    }

    uint32_t VulkanBindlessManager::RegisterSampler(SamplerHandle handle) {
        auto& context = m_GraphicsDevice->GetContext();
        auto& device = context.GetDevice();

        auto resource = m_ResourceManager->ResolveSampler(handle);

        vk::DescriptorImageInfo samplerInfo { .sampler = resource->sampler, .imageView = nullptr, .imageLayout = vk::ImageLayout::eUndefined };
        vk::WriteDescriptorSet descriptorWrite { .dstSet = *m_DescriptorSet,
                                                 .dstBinding = 1, // Binding 1
                                                 .dstArrayElement = handle.GetIndex(),
                                                 .descriptorCount = 1,
                                                 .descriptorType = vk::DescriptorType::eSampler,
                                                 .pImageInfo = &samplerInfo };
        device.updateDescriptorSets(descriptorWrite, nullptr);
        return handle.GetIndex();
    }

    uint32_t VulkanBindlessManager::RegisterCmpSampler(SamplerHandle handle) {
        auto& context = m_GraphicsDevice->GetContext();
        auto& device = context.GetDevice();

        auto resource = m_ResourceManager->ResolveSampler(handle);

        vk::DescriptorImageInfo samplerInfo { .sampler = resource->sampler, .imageView = nullptr, .imageLayout = vk::ImageLayout::eUndefined };
        vk::WriteDescriptorSet descriptorWrite{ .dstSet = *m_DescriptorSet,
                                                 .dstBinding = 3, // Binding 3
                                                 .dstArrayElement = handle.GetIndex(),
                                                 .descriptorCount = 1,
                                                 .descriptorType = vk::DescriptorType::eSampler,
                                                 .pImageInfo = &samplerInfo };
        device.updateDescriptorSets(descriptorWrite, nullptr);
        return handle.GetIndex();
    }

    void VulkanBindlessManager::UnregisterSampler(uint32_t bindlessIndex) {

    }

    void VulkanBindlessManager::UpdateSceneBuffer(vk::Buffer sceneBuffer, vk::DeviceSize size) {
        auto& context = m_GraphicsDevice->GetContext();
        auto& device = context.GetDevice();

        vk::DescriptorBufferInfo bufferInfo { .buffer = sceneBuffer, .offset = 0, .range = size };
        vk::WriteDescriptorSet   descriptorWrite { .dstSet = *m_DescriptorSet,
                                                   .dstBinding = 2, // Binding 2
                                                   .dstArrayElement = 0,
                                                   .descriptorCount = 1,
                                                   .descriptorType = vk::DescriptorType::eUniformBuffer,
                                                   .pBufferInfo = &bufferInfo };
        device.updateDescriptorSets(descriptorWrite, nullptr);
    }

    void VulkanBindlessManager::SetResourceManager(const Scope<ResourceManager>& resourceManager) {
        auto vulkanResourceManager = static_cast<VulkanResourceManager*>(resourceManager.get());
        m_ResourceManager = vulkanResourceManager;
    }

    vk::ImageLayout VulkanBindlessManager::ConvertLayout(TextureLayout layout) const {
        switch (layout) {
            case TextureLayout::GENERAL:          return vk::ImageLayout::eGeneral;
            case TextureLayout::COLOR_ATTACHMENT: return vk::ImageLayout::eColorAttachmentOptimal;
            case TextureLayout::DEPTH_ATTACHMENT: return vk::ImageLayout::eDepthStencilAttachmentOptimal;
            case TextureLayout::SHADER_READ:      return vk::ImageLayout::eShaderReadOnlyOptimal;
            case TextureLayout::TRANSFER_SRC:     return vk::ImageLayout::eTransferSrcOptimal;
            case TextureLayout::TRANSFER_DST:     return vk::ImageLayout::eTransferDstOptimal;
            case TextureLayout::PRESENT_SRC:      return vk::ImageLayout::ePresentSrcKHR;
            case TextureLayout::UNDEFINED:
            default:                              return vk::ImageLayout::eUndefined;
        }
    }

}