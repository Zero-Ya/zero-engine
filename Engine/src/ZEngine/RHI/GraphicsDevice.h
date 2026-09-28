#pragma once

#include "RHICommon.h"
#include "RHICommandList.h"

namespace ZEngine {

    class GraphicsDevice {
    public:
        virtual ~GraphicsDevice() = default;

        virtual void Init() = 0;
        virtual void Shutdown() = 0;

        virtual void OnResize(uint32_t width, uint32_t height) = 0;
        virtual void BeginFrame() = 0;
        virtual void EndFrame() = 0;
        virtual void WaitIdle() = 0;

        // RHI command and handle accessors 
        virtual RHICommandList& GetMainCommandList() = 0;
        virtual TextureHandle GetSwapchainTextureHandle() const = 0;
        virtual TextureHandle GetSwapchainDepthHandle() const = 0;

        // Manual resource allocation interfaces
        virtual BufferHandle CreateBuffer(const BufferCreateInfo& info) = 0;
        virtual void DestroyBuffer(BufferHandle handle) = 0;

        virtual TextureHandle CreateTexture(const TextureCreateInfo& info) = 0;
        virtual void DestroyTexture(TextureHandle handle) = 0;

        static Scope<GraphicsDevice> Create(void* window, uint32_t width, uint32_t height);
    };

}