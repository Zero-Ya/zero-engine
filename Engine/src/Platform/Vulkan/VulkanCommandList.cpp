#include "VulkanCommandList.h"

#include "VulkanGraphicsDevice.h"
#include "VulkanResourceManager.h"

namespace ZEngine {

    void VulkanCommandList::Init(VulkanGraphicsDevice* graphicsDevice) {
        m_GraphicsDevice = graphicsDevice;
    }

    void VulkanCommandList::SetTargetCommandBuffer(const vk::raii::CommandBuffer& commandBuffer) {
        m_CommandBuffer = commandBuffer;
    }

    void VulkanCommandList::SetResourceManager(const Scope<ResourceManager>& resourceManager) {
        auto vulkanResourceManager = static_cast<VulkanResourceManager*>(resourceManager.get());
        m_ResourceManager = vulkanResourceManager;
    }

    void VulkanCommandList::BeginRendering(const RenderingInfo& renderingInfo) {

        vk::ImageView colorImageView;
         vk::RenderingAttachmentInfo colorAttachmentInfo {};

        if (renderingInfo.hasColor) {
            if (renderingInfo.colorAttachments[0].texture.IsExternal()) {
                auto colorTextureResource = m_ResourceManager->ResolveExternal(renderingInfo.colorAttachments[0].texture);
                colorImageView = colorTextureResource->imageView;
            }
            else {
                auto colorTextureResource = m_ResourceManager->ResolveTexture(renderingInfo.colorAttachments[0].texture);
                colorImageView = colorTextureResource->imageView;
            }

            colorAttachmentInfo.imageView = colorImageView;
            colorAttachmentInfo.imageLayout = ConvertLayout(TextureLayout::COLOR_ATTACHMENT);
            colorAttachmentInfo.loadOp = renderingInfo.colorAttachments[0].clearOnLoad ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad;
            colorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
            colorAttachmentInfo.clearValue = vk::ClearColorValue{ 0.0f, 0.0f, 0.0f, 1.0f };
        }

        vk::ImageView depthImageView;
        vk::RenderingAttachmentInfo depthAttachmentInfo {};

        if (renderingInfo.hasDepth) {
            if (renderingInfo.depthAttachment.texture.IsExternal()) {
                auto depthTextureResource = m_ResourceManager->ResolveExternal(renderingInfo.depthAttachment.texture);
                depthImageView = depthTextureResource->imageView;
            }
            else {
                auto depthTextureResource = m_ResourceManager->ResolveTexture(renderingInfo.depthAttachment.texture);
                depthImageView = depthTextureResource->imageView;
            }

            depthAttachmentInfo.imageView = depthImageView;
            depthAttachmentInfo.imageLayout = ConvertLayout(TextureLayout::DEPTH_ATTACHMENT);
            depthAttachmentInfo.loadOp = renderingInfo.depthAttachment.clearOnLoad ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad;
            depthAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
            depthAttachmentInfo.clearValue = vk::ClearDepthStencilValue(1.0f, 0);
        }

        vk::RenderingInfo vkRenderingInfo {
            .renderArea = vk::Rect2D{ vk::Offset2D{0, 0}, { renderingInfo.renderWidth, renderingInfo.renderHeight } },
            .layerCount = 1,
            .colorAttachmentCount = renderingInfo.colorAttachmentCount,
            .pColorAttachments = renderingInfo.hasColor ? &colorAttachmentInfo : nullptr,
            .pDepthAttachment = renderingInfo.hasDepth ? &depthAttachmentInfo : nullptr
        };

        m_CommandBuffer.beginRendering(vkRenderingInfo);
    }

    void VulkanCommandList::EndRendering() {
        m_CommandBuffer.endRendering();
    }

    void VulkanCommandList::SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth) {
        vk::Viewport viewport { float(x), float(height), static_cast<float>(width), -(static_cast<float>(height)), minDepth, maxDepth };
        m_CommandBuffer.setViewport(0, { viewport });
    }

    void VulkanCommandList::SetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height) {
        vk::Rect2D scissor { { (int32_t)x, (int32_t)y }, { width, height } };
        m_CommandBuffer.setScissor(0, { scissor });
    }

    void VulkanCommandList::TransitionImageLayout(
        TextureHandle texture,
        TextureLayout oldTextureLayout,
        TextureLayout newTextureLayout
    ) {
        if (oldTextureLayout == newTextureLayout) return;

        auto [srcStage, srcAccess, oldLayout] = MapStateToVulkan(oldTextureLayout);
        auto [dstStage, dstAccess, newLayout] = MapStateToVulkan(newTextureLayout);

        vk::Image image; 
        vk::ImageAspectFlags aspectMask;

        if (texture.IsExternal()) {
            auto colorTextureResource = m_ResourceManager->ResolveExternal(texture);
            image = colorTextureResource->image;
            aspectMask = GetAspectFromFormat(colorTextureResource->format);
        }
        else {
            auto colorTextureResource = m_ResourceManager->ResolveTexture(texture);
            image = colorTextureResource->image;
            aspectMask = GetAspectFromFormat(colorTextureResource->format);
        }

        //vk::ImageAspectFlags aspectMask = (newTextureLayout == TextureLayout::DEPTH_ATTACHMENT)
        //    ? vk::ImageAspectFlagBits::eDepth
        //    : vk::ImageAspectFlagBits::eColor;

        vk::ImageMemoryBarrier2 barrier = {
            .srcStageMask = srcStage,
            .srcAccessMask = srcAccess,
            .dstStageMask = dstStage,
            .dstAccessMask = dstAccess,
            .oldLayout = oldLayout,
            .newLayout = newLayout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image,
            .subresourceRange = {
                   .aspectMask = aspectMask,
                   .baseMipLevel = 0,
                   .levelCount = 1,
                   .baseArrayLayer = 0,
                   .layerCount = 1} };
        vk::DependencyInfo dependency_info = {
            .dependencyFlags = {},
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &barrier };
        m_CommandBuffer.pipelineBarrier2(dependency_info);
    }

    void VulkanCommandList::BindPipeline(const PipelineHandle& pipelineHandle) {

    }

    void VulkanCommandList::BindVertexBuffer(BufferHandle buffer, uint64_t offset) {
        auto bufferResource = m_ResourceManager->ResolveBuffer(buffer);
        vk::Buffer vkBuffer = bufferResource->buffer;
        m_CommandBuffer.bindVertexBuffers(0, { vkBuffer }, { offset });
    }

    void VulkanCommandList::BindIndexBuffer(BufferHandle buffer, uint64_t offset, IndexType indexType) {
        auto bufferResource = m_ResourceManager->ResolveBuffer(buffer);
        vk::Buffer vkBuffer = bufferResource->buffer;
        vk::IndexType vkIndexType = (indexType == IndexType::UINT32) ? vk::IndexType::eUint32 : vk::IndexType::eUint16;
        m_CommandBuffer.bindIndexBuffer(vkBuffer, offset, vkIndexType);
    }

    void VulkanCommandList::PushConstants(uint32_t stageFlags, uint32_t offset, uint32_t size, const void* pValues) {
        //m_CommandBuffer.pushConstants(nullptr, static_cast<vk::ShaderStageFlags>(stageFlags), offset, size, pValues);
    }

    void VulkanCommandList::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
        m_CommandBuffer.drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    void VulkanCommandList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
        m_CommandBuffer.draw(vertexCount, instanceCount, firstVertex, firstInstance);
    }

    vk::ImageLayout VulkanCommandList::ConvertLayout(TextureLayout layout) const {
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
    // Uhhhh....
    VulkanStateMapping VulkanCommandList::MapStateToVulkan(TextureLayout layout) {
        switch (layout) {
            case TextureLayout::UNDEFINED:
                return { vk::PipelineStageFlagBits2::eTopOfPipe, vk::AccessFlagBits2::eNone, vk::ImageLayout::eUndefined };

            case TextureLayout::COLOR_ATTACHMENT:
                return { vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite, vk::ImageLayout::eColorAttachmentOptimal };

            case TextureLayout::SHADER_READ:
                return { vk::PipelineStageFlagBits2::eFragmentShader | vk::PipelineStageFlagBits2::eComputeShader, vk::AccessFlagBits2::eShaderRead, vk::ImageLayout::eReadOnlyOptimal };

            case TextureLayout::DEPTH_ATTACHMENT:
                return { vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests, vk::AccessFlagBits2::eDepthStencilAttachmentWrite, vk::ImageLayout::eDepthAttachmentOptimal };

            case TextureLayout::TRANSFER_SRC:
                return { vk::PipelineStageFlagBits2::eCopy, vk::AccessFlagBits2::eTransferRead, vk::ImageLayout::eTransferSrcOptimal };

            case TextureLayout::TRANSFER_DST:
                return { vk::PipelineStageFlagBits2::eCopy, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal };

            case TextureLayout::PRESENT_SRC:
                return { vk::PipelineStageFlagBits2::eBottomOfPipe, vk::AccessFlagBits2::eNone, vk::ImageLayout::ePresentSrcKHR };
            default:
                ZE_ERROR("Unsupported layout!");
                return {};
        }
    }

    vk::ImageAspectFlags VulkanCommandList::GetAspectFromFormat(vk::Format format) {
        switch (format) {
            case vk::Format::eD32Sfloat: return vk::ImageAspectFlagBits::eDepth;
            case vk::Format::eD24UnormS8Uint: return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
            default: return vk::ImageAspectFlagBits::eColor;
        }
    }

}