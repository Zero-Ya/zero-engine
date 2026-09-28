#pragma once

#include "RHICommon.h"

namespace ZEngine {

    class RHICommandList {
    public:
        virtual ~RHICommandList() = default;

        // Dynamic rendering commands
        virtual void BeginRendering(const TextureHandle& swapchainTexture, const TextureHandle& depthTexture) = 0;
        virtual void EndRendering(const TextureHandle& swapchainTexture) = 0;

        // Layout transitions and synchronization
        virtual void TransitionImageLayout(
            TextureHandle texture,
            TextureLayout oldLayout,
            TextureLayout newLayout
        ) = 0;

        // State commands
        virtual void SetViewport(float x, float y, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f) = 0;
        virtual void SetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height) = 0;
        //virtual void BindPipeline(PipelineHandle pipeline) = 0;

        // Draw/Bind commands
        virtual void BindVertexBuffer(BufferHandle buffer, uint64_t offset = 0) = 0;
        virtual void BindIndexBuffer(BufferHandle buffer, uint64_t offset = 0, IndexType indexType = IndexType::UINT32) = 0;
        virtual void PushConstants(uint32_t stageFlags, uint32_t offset, uint32_t size, const void* pValues) = 0;
        virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t vertexOffset = 0, uint32_t firstInstance = 0) = 0;
        virtual void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0) = 0;
    };

}