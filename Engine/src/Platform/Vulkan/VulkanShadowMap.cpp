#include "VulkanShadowMap.h"

#include "VulkanGraphicsDevice.h"
#include "VulkanContext.h"
#include "VulkanSwapchain.h"

#include "VulkanShader.h"

#include "VulkanBindlessManager.h"
#include "VulkanResourceManager.h"

namespace ZEngine {

    Ref<ShadowMap> ShadowMap::Create(const Scope<GraphicsDevice>& graphicsDevice, const Scope<BindlessDescriptorManager>& bindlessManager, const Scope<ResourceManager>& resourceManager) {
        return std::make_shared<VulkanShadowMap>(graphicsDevice, bindlessManager, resourceManager);
    }

    VulkanShadowMap::VulkanShadowMap(const Scope<GraphicsDevice>& graphicsDevice, const Scope<BindlessDescriptorManager>& bindlessManager, const Scope<ResourceManager>& resourceManager) {
        m_GraphicsDevice = static_cast<VulkanGraphicsDevice*>(graphicsDevice.get());
        m_ResourceManager = static_cast<VulkanResourceManager*>(resourceManager.get());
        m_BindlessManager = static_cast<VulkanBindlessManager*>(bindlessManager.get());
    }

    TextureHandle VulkanShadowMap::Init(const PipelineSpecification& spec, uint32_t shadowResolution) {
        auto& context = m_GraphicsDevice->GetContext();
        auto& device = context.GetDevice();
        auto& physicalDevice = context.GetPhysicalDevice();

        TextureCreateInfo textureInfo {
            .width = static_cast<uint32_t>(shadowResolution),
            .height = static_cast<uint32_t>(shadowResolution),
            .format = TextureFormat::D32_FLOAT,
            .usageFlags = TextureUsageFlags::DepthAttachment | TextureUsageFlags::Sampled,
            .levelCount = 1
        };
        TextureHandle imageHandle = m_ResourceManager->CreateShadowMap(textureInfo);

        auto textureIndex = RegisterTexture(imageHandle, TextureLayout::SHADER_READ);
        auto samplerIndex = RegisterSampler();

        m_TextureIndex = textureIndex;
        m_SamplerIndex = samplerIndex;

        CreatePipelineState(spec);

        return imageHandle;
    }

    void VulkanShadowMap::CreatePipelineState(const PipelineSpecification& spec) {
        auto& context = m_GraphicsDevice->GetContext();

        auto vk_Shader = static_cast<VulkanShader*>(spec.Shader.get());
        auto& device = context.GetDevice();

        // Shader stages info
        std::vector<vk::PipelineShaderStageCreateInfo> shaderStages = vk_Shader->GetShaderStages();

        // Vertex input info
        uint32_t stride = 0;
        for (const auto& element : spec.Layout) {
            stride += element.Size;
        }
        vk::VertexInputBindingDescription bindingDescription(0, 32, vk::VertexInputRate::eVertex);

        std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
        uint32_t location = 0;
        uint32_t offset = 0;
        for (const auto& element : spec.Layout) {
            attributeDescriptions.push_back({ location++, 0, ShaderDataTypeToVulkanFormat(element.Type), offset });
            offset += element.Size;
        }
        vk::PipelineVertexInputStateCreateInfo vertexInputInfo { .vertexBindingDescriptionCount = 1,
                                                                 .pVertexBindingDescriptions = &bindingDescription,
                                                                 .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
                                                                 .pVertexAttributeDescriptions = attributeDescriptions.data() };

        vk::PipelineInputAssemblyStateCreateInfo inputAssembly{ .topology = vk::PrimitiveTopology::eTriangleList };

        // Dynamic viewport / scissor
        std::vector<vk::DynamicState> dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
        vk::PipelineDynamicStateCreateInfo dynamicState{ .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()), .pDynamicStates = dynamicStates.data() };

        vk::PipelineViewportStateCreateInfo viewportState{ .viewportCount = 1, .scissorCount = 1 };

        // Rasterizer setup (depth bias + front face culling to stop acne)
        vk::PipelineRasterizationStateCreateInfo rasterizerInfo { .depthClampEnable = vk::True,
                                                                  .rasterizerDiscardEnable = vk::False,
                                                                  .polygonMode = vk::PolygonMode::eFill,
                                                                  .cullMode = vk::CullModeFlagBits::eFront,
                                                                  .frontFace = vk::FrontFace::eCounterClockwise,
                                                                  .depthBiasEnable = vk::True,
                                                                  .depthBiasConstantFactor = 1.25f,
                                                                  .depthBiasSlopeFactor = 1.75f,
                                                                  .lineWidth = 1.0f };

        vk::PipelineMultisampleStateCreateInfo multisampling{ .rasterizationSamples = vk::SampleCountFlagBits::e1 };

        // Depth/stencil state
        vk::PipelineDepthStencilStateCreateInfo depthStencil{
            .depthTestEnable = vk::True,
            .depthWriteEnable = vk::True,
            .depthCompareOp = vk::CompareOp::eLessOrEqual
        };

        // Push constant range
        vk::PushConstantRange pushConstantRange(
            vk::ShaderStageFlagBits::eVertex,
            0,
            sizeof(ShadowPushConstants)
        );

        auto set0Layout = m_BindlessManager->GetSet0Layout();
        auto set1Layout = m_BindlessManager->GetSet1Layout();
        vk::DescriptorSetLayout layouts[] = { set0Layout, set1Layout };

        vk::PipelineLayoutCreateInfo layoutInfo {
            .setLayoutCount = 2,
            .pSetLayouts = layouts,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &pushConstantRange
        };

        // Pipeline layout
        m_PipelineLayout = vk::raii::PipelineLayout(device, layoutInfo);

        // Dynamic rendering target format info (depth-only format)
        vk::Format depthFormat = vk::Format::eD32Sfloat;
        vk::PipelineRenderingCreateInfo dynamicRenderingInfo{ .depthAttachmentFormat = depthFormat };

        // Create graphics pipeline
        vk::GraphicsPipelineCreateInfo pipelineInfo{
            .pNext = &dynamicRenderingInfo,
            .stageCount = 2,
            .pStages = shaderStages.data(),
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssembly,
            .pViewportState = &viewportState,
            .pRasterizationState = &rasterizerInfo,
            .pMultisampleState = &multisampling,
            .pDepthStencilState = &depthStencil,
            .pColorBlendState = nullptr,
            .pDynamicState = &dynamicState,
            .layout = m_PipelineLayout
        };

        m_Pipeline = vk::raii::Pipeline(device, nullptr, pipelineInfo);
    }

    uint32_t VulkanShadowMap::RegisterTexture(TextureHandle handle, const TextureLayout& layout) {
        auto bindlessIndex = m_BindlessManager->RegisterTexture(handle, layout);
        return bindlessIndex;
    }

    uint32_t VulkanShadowMap::RegisterSampler() {
        // Create PCF depth comparison sampler
        SamplerHandle samplerHandle = m_ResourceManager->CreateSampler({
            .magFilter = vk::Filter::eLinear,
            .minFilter = vk::Filter::eLinear,
            .mipmapMode = vk::SamplerMipmapMode::eLinear,
            .addressModeU = vk::SamplerAddressMode::eClampToEdge,
            .addressModeV = vk::SamplerAddressMode::eClampToBorder,
            .addressModeW = vk::SamplerAddressMode::eClampToBorder,
            .enableCompare = vk::True,
            .compareOp = vk::CompareOp::eLessOrEqual,
            .borderColor = vk::BorderColor::eFloatOpaqueWhite
        });

        // Register
        auto bindlessIndex = m_BindlessManager->RegisterCmpSampler(samplerHandle);
        return bindlessIndex;
    }

}