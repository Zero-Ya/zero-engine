#pragma once

#include "ZEngine/RHI/GraphicsDevice.h"

#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>

namespace ZEngine {

    class VulkanContext;
    class VulkanSwapchain;
    class VulkanCommandList;

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

        VulkanContext& GetContext() { return *m_Context; }
        VulkanSwapchain& GetSwapchain() { return *m_Swapchain; }

        const uint32_t& GetCurrentImageIndex() const { return m_CurrentImageIndex; }

    private:
        void RecreateSwapchain();

        Scope<VulkanContext> m_Context;
        Scope<VulkanSwapchain> m_Swapchain;
        Scope<VulkanCommandList> m_CommandList;

        uint32_t m_CurrentImageIndex = 0;
        bool m_FramebufferResized = false;
        uint32_t m_Width = 0;
        uint32_t m_Height = 0;
    };

}