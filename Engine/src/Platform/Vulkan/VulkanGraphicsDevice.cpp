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

        // Cleanup buffers, can we use the DestroyBuffer instead somehow?
        for (auto& [id, buf] : m_Buffers) {
            delete buf.buffer;
            delete buf.memory;
        }
        m_Buffers.clear();

        // Cleanup textures
        for (auto& [id, tex] : m_Textures) {
            // Skip reserved swapchain pseudo-handles (owned by VulkanSwapchain)
            if (id == SWAPCHAIN_TEXTURE_HANDLE.id || id == SWAPCHAIN_DEPTH_HANDLE.id) continue;
            delete tex.imageView;
            delete tex.image;
            delete tex.memory;
        }
        m_Textures.clear();

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

        m_Textures[SWAPCHAIN_TEXTURE_HANDLE.id] = VulkanTextureResource {
            .image = m_Swapchain->GetImage(imageIndex),
            .memory = nullptr,
            .imageView = m_Swapchain->GetImageView(imageIndex),
            .width = m_Swapchain->GetExtent().width,
            .height = m_Swapchain->GetExtent().height,
            .format = static_cast<uint32_t>(m_Swapchain->GetSurfaceFormat().format)
        };

        m_Textures[SWAPCHAIN_DEPTH_HANDLE.id] = VulkanTextureResource {
            .image = m_Swapchain->GetDepthImage(),
            .memory = nullptr,
            .imageView = m_Swapchain->GetDepthImageView(),
            .width = m_Swapchain->GetExtent().width,
            .height = m_Swapchain->GetExtent().height,
            .format = static_cast<uint32_t>(m_Swapchain->GetDepthFormat())
        };

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

    uint32_t VulkanGraphicsDevice::FindMemoryType(uint32_t typeFilter, uint32_t propertyFlags) const {
        auto memProperties = m_Context->GetPhysicalDevice().getMemoryProperties();
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & static_cast<vk::MemoryPropertyFlags>(propertyFlags)) == static_cast<vk::MemoryPropertyFlags>(propertyFlags)) {
                return i;
            }
        }
        throw std::runtime_error("Failed to find suitable Vulkan memory type!");
    }

    BufferHandle VulkanGraphicsDevice::CreateBuffer(const BufferCreateInfo& info) {
        vk::BufferUsageFlags usage = static_cast<vk::BufferUsageFlagBits>(info.usageFlags);

        vk::BufferCreateInfo createInfo {
            .size = info.size,
            .usage = usage,
            .sharingMode = vk::SharingMode::eExclusive
        };

        vk::raii::Buffer buffer = vk::raii::Buffer(m_Context->GetDevice(), createInfo);
        vk::MemoryRequirements memReqs = buffer.getMemoryRequirements();

        vk::MemoryPropertyFlags properties;
        if (info.isHostVisible) {
            properties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
        }
        else {
            properties = vk::MemoryPropertyFlagBits::eDeviceLocal;
        }

        vk::MemoryAllocateInfo allocInfo {
            .allocationSize = memReqs.size,
            .memoryTypeIndex = FindMemoryType(memReqs.memoryTypeBits, static_cast<uint32_t>(properties))
        };

        vk::raii::DeviceMemory memory = vk::raii::DeviceMemory(m_Context->GetDevice(), allocInfo);
        buffer.bindMemory(*memory, 0);

        void* mappedPtr = nullptr;
        if (info.isHostVisible) {
            mappedPtr = memory.mapMemory(0, info.size);
        }

        uint64_t handleId = m_NextBufferId++;
        m_Buffers[handleId] = VulkanBufferResource {
            .buffer = std::move(buffer),
            .memory = std::move(memory),
            .size = info.size,
            .mappedData = mappedPtr
        };

        return BufferHandle { handleId };
    }

    TextureHandle VulkanGraphicsDevice::CreateTexture(const TextureCreateInfo& info) {
        // Manual texture creation logic...
        return TextureHandle { 0 };
    }

    void VulkanGraphicsDevice::DestroyBuffer(BufferHandle handle) {
        auto it = m_Buffers.find(handle.id);
        if (it != m_Buffers.end()) {
            delete it->second.buffer;
            delete it->second.memory;
            m_Buffers.erase(it);
        }
    }

    void VulkanGraphicsDevice::DestroyTexture(TextureHandle handle) {
        auto it = m_Textures.find(handle.id);
        if (it != m_Textures.end()) {
            delete it->second.imageView;
            delete it->second.image;
            delete it->second.memory;
            m_Textures.erase(it);
        }
    }

    VulkanBufferResource* VulkanGraphicsDevice::ResolveBuffer(BufferHandle handle) {
        auto it = m_Buffers.find(handle.id);
        return (it != m_Buffers.end()) ? &it->second : nullptr;
    }

    VulkanTextureResource* VulkanGraphicsDevice::ResolveTexture(TextureHandle handle) {
        auto it = m_Textures.find(handle.id);
        return (it != m_Textures.end()) ? &it->second : nullptr;
    }

}