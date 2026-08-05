#include "VulkanRendererAPI.h"
#include "ZEngine/Core/Application.h"
#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanSwapchain.h"
#include "Platform/Vulkan/VulkanCommandBuffer.h"
#include "Platform/Vulkan/VulkanVertexArray.h"
#include "Platform/Vulkan/VulkanBuffer.h"
#include "VulkanPipelineState.h"

namespace ZEngine {

	void VulkanRendererAPI::Init() {
        
	}

    void VulkanRendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
        if (!m_ActiveCommandBuffer) return;

        auto vulkanCommandBuffer = static_cast<VulkanCommandBuffer*>(m_ActiveCommandBuffer.get());
        const auto& commandBuffer = vulkanCommandBuffer->GetBuffer();

        vk::Viewport viewport(float(x), float(height), static_cast<float>(width), -(static_cast<float>(height)), 0.0f, 1.0f);
        vk::Rect2D scissor({ (int32_t)x, (int32_t)y }, {width, height});

        commandBuffer.setViewport(0, viewport);
        commandBuffer.setScissor(0, scissor);
    }

    void VulkanRendererAPI::SetClearColor(const glm::vec4& color) {
        m_ClearColor = color;
    }

    void VulkanRendererAPI::Clear() {
        // No need to do anything
    }

    void VulkanRendererAPI::BeginFrame(const std::shared_ptr<RenderCommandBuffer>& renderCommandBuffer, uint32_t imageIndex) {
        m_ActiveCommandBuffer = renderCommandBuffer;
        m_CurrentImageIndex = imageIndex;

        auto vk_Context = static_cast<VulkanContext*>(Application::Get().GetGraphicsContext());
        auto swapchain = vk_Context->GetSwapchain();
        auto vulkanCommandBuffer = static_cast<VulkanCommandBuffer*>(m_ActiveCommandBuffer.get());
        const auto& commandBuffer = vulkanCommandBuffer->GetBuffer();

        TransitionImageLayout(
            swapchain->GetImage(m_CurrentImageIndex),
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal,
            {},
            vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::ImageAspectFlagBits::eColor
        );

        vk::ClearValue clearValue = {
            vk::ClearColorValue(std::array<float, 4>{ m_ClearColor.r, m_ClearColor.g, m_ClearColor.b, m_ClearColor.a })
        };

        vk::RenderingAttachmentInfo colorAttachment {
            .imageView = swapchain->GetImageView(m_CurrentImageIndex),
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = clearValue
        };

        const vk::Extent2D& extent = swapchain->GetExtent();

        vk::RenderingInfo renderingInfo {
            .flags = vk::RenderingFlags{},
            .renderArea = vk::Rect2D{vk::Offset2D{0, 0}, extent},
            .layerCount = 1,
            .viewMask = 0,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachment,
            .pDepthAttachment = nullptr,
            .pStencilAttachment = nullptr
        };

        // Begin the dynamic rendering block
        commandBuffer.beginRendering(renderingInfo);
    }

    void VulkanRendererAPI::EndFrame() {
        if (!m_ActiveCommandBuffer) return;

        auto vk_Context = static_cast<VulkanContext*>(Application::Get().GetGraphicsContext());
        auto swapchain = vk_Context->GetSwapchain();
        auto vulkanCommandBuffer = static_cast<VulkanCommandBuffer*>(m_ActiveCommandBuffer.get());
        const auto& commandBuffer = vulkanCommandBuffer->GetBuffer();

        // End the dynamic rendering block
        commandBuffer.endRendering();

        // After rendering, transition the swapchain image to vk::ImageLayout::ePresentSrcKHR
        TransitionImageLayout(
            swapchain->GetImage(m_CurrentImageIndex),
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::ePresentSrcKHR,
            vk::AccessFlagBits2::eColorAttachmentWrite,
            {},
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::PipelineStageFlagBits2::eBottomOfPipe,
            vk::ImageAspectFlagBits::eColor
        );
    }

    void VulkanRendererAPI::Shutdown() {
    }

    void VulkanRendererAPI::DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount) {
        auto vulkanCommandBuffer = static_cast<VulkanCommandBuffer*>(m_ActiveCommandBuffer.get());
        const auto& commandBuffer = vulkanCommandBuffer->GetBuffer();

        auto vulkanVertexArray = static_cast<VulkanVertexArray*>(vertexArray.get());
        vulkanVertexArray->BindToCommandBuffer(commandBuffer);

        uint32_t count = indexCount == 0 ? vertexArray->GetIndexBuffer()->GetCount() : indexCount;

        commandBuffer.drawIndexed(count, 1, 0, 0, 0);
    }

    // We can probably combine this and the one below or something
    void VulkanRendererAPI::BindPipelineState(const std::shared_ptr<PipelineState>& pipelineState) {
        auto vulkanCommandBuffer = static_cast<VulkanCommandBuffer*>(m_ActiveCommandBuffer.get());
        const auto& commandBuffer = vulkanCommandBuffer->GetBuffer();
        auto vulkanPipeline = static_cast<VulkanPipelineState*>(pipelineState.get());

        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, vulkanPipeline->GetNativePipeline());
    }

    void VulkanRendererAPI::BindDescriptorSets(const std::shared_ptr<PipelineState>& pipelineState, const std::shared_ptr<UniformBuffer>& ubo) {
        auto vulkanCommandBuffer = static_cast<VulkanCommandBuffer*>(m_ActiveCommandBuffer.get());
        const auto& commandBuffer = vulkanCommandBuffer->GetBuffer();

        auto vulkanPipeline = static_cast<VulkanPipelineState*>(pipelineState.get());
        auto vulkanUBO = static_cast<VulkanUniformBuffer*>(ubo.get());

        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, vulkanPipeline->GetNativeLayout(), 0, *vulkanUBO->GetFrameDescriptorSet(), nullptr);
    }

    void VulkanRendererAPI::TransitionImageLayout(
        vk::Image               image,
        vk::ImageLayout         old_layout,
        vk::ImageLayout         new_layout,
        vk::AccessFlags2        src_access_mask,
        vk::AccessFlags2        dst_access_mask,
        vk::PipelineStageFlags2 src_stage_mask,
        vk::PipelineStageFlags2 dst_stage_mask,
        vk::ImageAspectFlags    image_aspect_flags)
    {
        auto vulkanCommandBuffer = static_cast<VulkanCommandBuffer*>(m_ActiveCommandBuffer.get());
        const auto& commandBuffer = vulkanCommandBuffer->GetBuffer();

        vk::ImageMemoryBarrier2 barrier = {
            .srcStageMask = src_stage_mask,
            .srcAccessMask = src_access_mask,
            .dstStageMask = dst_stage_mask,
            .dstAccessMask = dst_access_mask,
            .oldLayout = old_layout,
            .newLayout = new_layout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image,
            .subresourceRange = {
                   .aspectMask = image_aspect_flags,
                   .baseMipLevel = 0,
                   .levelCount = 1,
                   .baseArrayLayer = 0,
                   .layerCount = 1} };
        vk::DependencyInfo dependency_info = {
            .dependencyFlags = {},
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &barrier };
        commandBuffer.pipelineBarrier2(dependency_info);
    }

}