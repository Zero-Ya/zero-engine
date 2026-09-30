#pragma once

#include "ZEngine/RHI/RHICommandList.h"

#include <vulkan/vulkan_raii.hpp>

namespace ZEngine {

    struct VulkanStateMapping {
        vk::PipelineStageFlags2 stage;
        vk::AccessFlags2 access;
        vk::ImageLayout layout;
    };

    class VulkanGraphicsDevice;

    class VulkanCommandList : public RHICommandList {
    public:
        VulkanCommandList() = default;
        ~VulkanCommandList() = default;

        void Init(VulkanGraphicsDevice* graphicsDevice);

        // Call at the start of each frame to point to the active frame's command buffer
        void SetTargetCommandBuffer(const vk::raii::CommandBuffer& commandBuffer);

        // --- RHICommandList Overrides ---
        void BeginRendering(const TextureHandle& swapchainTexture, const TextureHandle& depthTexture) override;
        void EndRendering() override;

        void SetViewport(float x, float y, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f) override;
        void SetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height) override;

        void TransitionImageLayout(
            TextureHandle texture,
            TextureLayout oldLayout,
            TextureLayout newLayout
        ) override;

        void BindVertexBuffer(BufferHandle buffer, uint64_t offset = 0) override;
        void BindIndexBuffer(BufferHandle buffer, uint64_t offset = 0, IndexType indexType = IndexType::UINT32) override;
        void PushConstants(uint32_t stageFlags, uint32_t offset, uint32_t size, const void* pValues) override;
        void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t vertexOffset = 0, uint32_t firstInstance = 0) override;
        void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0) override;

    private:
        void TransitionImageLayout(vk::Image               image,
                                   vk::ImageLayout         old_layout,
                                   vk::ImageLayout         new_layout,
                                   vk::AccessFlags2        src_access_mask,
                                   vk::AccessFlags2        dst_access_mask,
                                   vk::PipelineStageFlags2 src_stage_mask,
                                   vk::PipelineStageFlags2 dst_stage_mask,
                                   vk::ImageAspectFlags    image_aspect_flags);

        vk::ImageLayout ConvertLayout(TextureLayout layout) const;
        VulkanStateMapping MapStateToVulkan(TextureLayout layout);

        vk::CommandBuffer m_CommandBuffer = nullptr;
        VulkanGraphicsDevice* m_GraphicsDevice = nullptr;
    };

}