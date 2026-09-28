#pragma once

#include "ZEngine/RHI/GraphicsDevice.h"

#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>

namespace ZEngine {

    class VulkanContext;
    class VulkanSwapchain;
    class VulkanCommandList;

    struct VulkanBufferResource {
        vk::Buffer buffer       = nullptr;
        vk::DeviceMemory memory = nullptr;
        uint64_t size           = 0;
        void* mappedData        = nullptr;
    };

    struct VulkanTextureResource {
        vk::Image image         = nullptr;
        vk::DeviceMemory memory = nullptr;
        vk::ImageView imageView = nullptr;
        uint32_t width          = 0;
        uint32_t height         = 0;
        uint32_t format         = 0; // Enum from RHI common
    };

    class VulkanGraphicsDevice : public GraphicsDevice {
    public:
        VulkanGraphicsDevice(GLFWwindow* window, uint32_t width, uint32_t height);
        ~VulkanGraphicsDevice() override;

        void Init() override;
        void Shutdown() override;

        void OnResize(uint32_t width, uint32_t height) override;
        void BeginFrame() override;
        void EndFrame() override;
        void WaitIdle() override;

        RHICommandList& GetMainCommandList() override;
        TextureHandle GetSwapchainTextureHandle() const override { return SWAPCHAIN_TEXTURE_HANDLE; }
        TextureHandle GetSwapchainDepthHandle() const override { return SWAPCHAIN_DEPTH_HANDLE; }

        BufferHandle CreateBuffer(const BufferCreateInfo& info) override;
        void DestroyBuffer(BufferHandle handle) override;

        TextureHandle CreateTexture(const TextureCreateInfo& info) override;
        void DestroyTexture(TextureHandle handle) override;

        VulkanBufferResource* ResolveBuffer(BufferHandle handle);
        VulkanTextureResource* ResolveTexture(TextureHandle handle);

        VulkanContext& GetContext() { return *m_Context; }
        VulkanSwapchain& GetSwapchain() { return *m_Swapchain; }

    private:
        uint32_t FindMemoryType(uint32_t typeFilter, uint32_t propertyFlags) const;
        void RecreateSwapchain();

        Scope<VulkanContext> m_Context;
        Scope<VulkanSwapchain> m_Swapchain;
        Scope<VulkanCommandList> m_CommandList;

        uint32_t m_CurrentImageIndex = 0;
        bool m_FramebufferResized = false;
        uint32_t m_Width = 0;
        uint32_t m_Height = 0;

        uint64_t m_NextBufferId = 1;
        uint64_t m_NextTextureId = 1;
        std::unordered_map<uint64_t, VulkanBufferResource> m_Buffers;
        std::unordered_map<uint64_t, VulkanTextureResource> m_Textures;

    };

}