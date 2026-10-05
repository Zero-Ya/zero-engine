#include "VulkanResourceManager.h"

#include "VulkanGraphicsDevice.h"
#include "VulkanContext.h"
#include "VulkanSwapchain.h"

namespace ZEngine {

	Scope<ResourceManager> ResourceManager::Create(const Scope<GraphicsDevice>& graphicsDevice) {
		return std::make_unique<VulkanResourceManager>(graphicsDevice);
	}

    VulkanResourceManager::VulkanResourceManager(const Scope<GraphicsDevice>& graphicsDevice) {
        m_GraphicsDevice = static_cast<VulkanGraphicsDevice*>(graphicsDevice.get());

        InitRenderingHandles();
    }

    void VulkanResourceManager::InitRenderingHandles() {
        auto& swapchain = m_GraphicsDevice->GetSwapchain();
        auto imageIndex = m_GraphicsDevice->GetCurrentImageIndex();

        m_Textures[SWAPCHAIN_TEXTURE_HANDLE.id] = VulkanTextureResource{
            .image = swapchain.GetImage(imageIndex),
            .imageView = swapchain.GetImageView(imageIndex),
            .memory = nullptr,
            .format = swapchain.GetSurfaceFormat().format,
            .width = swapchain.GetExtent().width,
            .height = swapchain.GetExtent().height,
        };

        m_Textures[SWAPCHAIN_DEPTH_HANDLE.id] = VulkanTextureResource{
            .image = swapchain.GetDepthImage(),
            .imageView = swapchain.GetDepthImageView(),
            .memory = nullptr,
            .format = swapchain.GetDepthFormat(),
            .width = swapchain.GetExtent().width,
            .height = swapchain.GetExtent().height
        };
    }

    BufferHandle VulkanResourceManager::CreateBuffer(const BufferCreateInfo& info) {
        auto& context = m_GraphicsDevice->GetContext();

        auto size = info.size;
        auto usage = static_cast<vk::BufferUsageFlagBits>(info.usageFlags);

        // Create buffer
        vk::BufferCreateInfo createInfo {
            .size = size,
            .usage = usage,
            .sharingMode = vk::SharingMode::eExclusive
        };
        auto buffer = vk::raii::Buffer(context.GetDevice(), createInfo);

        // Allocate device memory
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

        auto memory = vk::raii::DeviceMemory(context.GetDevice(), allocInfo);
        buffer.bindMemory(*memory, 0);

        void* mappedPtr = nullptr;
        if (info.isHostVisible) {
            mappedPtr = memory.mapMemory(0, size);
        }

        // Create buffer resource for buffer handle
        uint64_t handleId = m_NextBufferId++;
        m_Buffers[handleId] = VulkanBufferResource{
            .buffer = std::move(buffer),
            .memory = std::move(memory),
            .size = size,
            .mappedData = mappedPtr
        };
        return BufferHandle{ handleId };
    }

    TextureHandle VulkanResourceManager::CreateTexture(const TextureCreateInfo& info) {
        auto& context = m_GraphicsDevice->GetContext();

        auto format = ConvertTextureFormat(info.format);
        auto width = info.width;
        auto height = info.height;
        auto usage = ConvertTextureUsageFlags(info.usageFlags);

        // Create image
        vk::ImageCreateInfo imageInfo { .imageType = vk::ImageType::e2D,
                                        .format = format,
                                        .extent = { width, height, 1 },
                                        .mipLevels = 1,
                                        .arrayLayers = 1,
                                        .samples = vk::SampleCountFlagBits::e1,
                                        .tiling = vk::ImageTiling::eOptimal,
                                        .usage = usage,
                                        .sharingMode = vk::SharingMode::eExclusive };
        auto image = vk::raii::Image(context.GetDevice(), imageInfo);

        // Allocate device memory
        vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
        vk::MemoryAllocateInfo allocInfo { .allocationSize = memRequirements.size, .memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, static_cast<uint32_t>(vk::MemoryPropertyFlagBits::eDeviceLocal)) };

        auto memory = vk::raii::DeviceMemory(context.GetDevice(), allocInfo);
        image.bindMemory(memory, 0);

        // Create image view
        vk::ImageViewCreateInfo viewInfo {
            .image = image,
            .viewType = vk::ImageViewType::e2D,
            .format = format,
            .subresourceRange = { .aspectMask = vk::ImageAspectFlagBits::eColor, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1 }
        };
        auto imageView = vk::raii::ImageView(context.GetDevice(), viewInfo);

        // Create texture resource for texture handle
        uint64_t textureId = m_NextTextureId++;
        m_Textures[textureId] = VulkanTextureResource {
            .image = std::move(image),
            .imageView = std::move(imageView),
            .memory = std::move(memory),
            .format = format,
            .width = width,
            .height = height
        };
        return TextureHandle { textureId };
    }

    void VulkanResourceManager::DestroyBuffer(BufferHandle handle) {
        auto it = m_Buffers.find(handle.id);
        if (it != m_Buffers.end()) {
            delete it->second.buffer;
            delete it->second.memory;
            m_Buffers.erase(it);
        }
    }

    void VulkanResourceManager::DestroyTexture(TextureHandle handle) {
        auto it = m_Textures.find(handle.id);
        if (it != m_Textures.end()) {
            delete it->second.imageView;
            delete it->second.image;
            delete it->second.memory;
            m_Textures.erase(it);
        }
    }

    VulkanBufferResource* VulkanResourceManager::ResolveBuffer(BufferHandle handle) {
        auto it = m_Buffers.find(handle.id);
        return (it != m_Buffers.end()) ? &it->second : nullptr;
    }

    VulkanTextureResource* VulkanResourceManager::ResolveTexture(TextureHandle handle) {
        auto it = m_Textures.find(handle.id);
        return (it != m_Textures.end()) ? &it->second : nullptr;
    }

    uint32_t VulkanResourceManager::FindMemoryType(uint32_t typeFilter, uint32_t propertyFlags) const {
        auto& context = m_GraphicsDevice->GetContext();
        auto memProperties = context.GetPhysicalDevice().getMemoryProperties();
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & static_cast<vk::MemoryPropertyFlags>(propertyFlags)) == static_cast<vk::MemoryPropertyFlags>(propertyFlags)) {
                return i;
            }
        }
        throw std::runtime_error("Failed to find suitable Vulkan memory type!");
    }

    vk::Format VulkanResourceManager::ConvertTextureFormat(TextureFormat format) {
        switch (format) {
            case TextureFormat::UNDEFINED:         return vk::Format::eUndefined;
            case TextureFormat::RGBA8_UNORM:       return vk::Format::eR8G8B8A8Unorm;
            case TextureFormat::BGRA8_UNORM:       return vk::Format::eB8G8R8A8Unorm;
            case TextureFormat::RGBA8_SRGB:        return vk::Format::eR8G8B8A8Srgb;
            case TextureFormat::BGRA8_SRGB:        return vk::Format::eB8G8R8A8Srgb;
            case TextureFormat::D32_FLOAT:         return vk::Format::eD32Sfloat;
            case TextureFormat::D24_UNORM_S8_UINT: return vk::Format::eD24UnormS8Uint;
            default:                               return vk::Format::eUndefined;
        }
    }

    vk::ImageUsageFlagBits VulkanResourceManager::ConvertTextureUsageFlags(TextureUsageFlags flag) {
        switch (flag) {
            case TextureUsageFlags::Sampled: return vk::ImageUsageFlagBits::eSampled;
            case TextureUsageFlags::ColorAttachment: return vk::ImageUsageFlagBits::eColorAttachment;
            case TextureUsageFlags::DepthAttachment: return vk::ImageUsageFlagBits::eDepthStencilAttachment;
            case TextureUsageFlags::TransferSrc: return vk::ImageUsageFlagBits::eTransferSrc;
            case TextureUsageFlags::TransferDst: return vk::ImageUsageFlagBits::eTransferDst;
            default:                             return vk::ImageUsageFlagBits::eColorAttachment;
        }
    }



}