#include "VulkanGraphicsDevice.h"

#include "VulkanContext.h"
#include "VulkanSwapchain.h"
#include "VulkanCommandList.h"

namespace ZEngine {

    Scope<GraphicsDevice> GraphicsDevice::Create(void* window, uint32_t width, uint32_t height) {
        return std::make_unique<VulkanGraphicsDevice>(static_cast<GLFWwindow*>(window), width, height);
    }

    VulkanGraphicsDevice::VulkanGraphicsDevice(GLFWwindow* window, uint32_t width, uint32_t height) {
        m_Width = width;
        m_Height = height;

        m_Context = std::make_unique<VulkanContext>(window);
        m_Swapchain = std::make_unique<VulkanSwapchain>();
        m_CommandList = std::make_unique<VulkanCommandList>();
    }

    VulkanGraphicsDevice::~VulkanGraphicsDevice() {
        Shutdown();
    }

    void VulkanGraphicsDevice::Init() {
        m_Context->Init();
        m_Swapchain->Create(m_Context->GetDevice(), m_Context->GetPhysicalDevice(), m_Context->GetSurface(), m_Width, m_Height);
        m_Context->CreateFrameResources();
        m_CommandList->Init(this);
    }

    void VulkanGraphicsDevice::Shutdown() {
        WaitIdle();

        m_Swapchain->Cleanup();
        //m_Context->Cleanup();
    }

    void VulkanGraphicsDevice::OnResize(uint32_t width, uint32_t height) {
        m_FramebufferResized = true;
        m_Width = width;
        m_Height = height;
    }

    void VulkanGraphicsDevice::RecreateSwapchain() {
        if (m_Width == 0 || m_Height == 0) return; // Paused (minimized window)

        m_Context->GetDevice().waitIdle();
        m_Swapchain->Recreate(m_Context->GetDevice(), m_Context->GetPhysicalDevice(), m_Context->GetSurface(), m_Width, m_Height);
        m_FramebufferResized = false;
    }

    void VulkanGraphicsDevice::BeginFrame() {
        if (m_FramebufferResized)
            RecreateSwapchain();

        m_Context->BeginFrame();

        auto& frame = m_Context->GetCurrentFrame();

        auto [result, imageIndex] = m_Swapchain->GetHandle().acquireNextImage(
            UINT64_MAX, *frame.imageAvailableSemaphore, nullptr);
        if (result == vk::Result::eErrorOutOfDateKHR) {
            RecreateSwapchain();
            return;
        }
        if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
            assert(result == vk::Result::eTimeout || result == vk::Result::eNotReady);
            ZE_CORE_ERROR("Failed to acquire swap chain image!");
        }

        m_CurrentImageIndex = imageIndex;

        m_CommandList->SetTargetCommandBuffer(frame.commandBuffer);
    }

    void VulkanGraphicsDevice::EndFrame() {
        m_Context->EndFrame(m_CurrentImageIndex);

        auto result = m_Context->Present(m_CurrentImageIndex, m_Swapchain->GetHandle());
        if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || m_FramebufferResized) {
            RecreateSwapchain();
        }
    }

    void VulkanGraphicsDevice::WaitIdle() {
        if (*m_Context->GetDevice()) {
            m_Context->WaitIdle();
        }
    }

    RHICommandList& VulkanGraphicsDevice::GetMainCommandList() {
        return *m_CommandList;
    }

}