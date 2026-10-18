#include "VulkanResourceManager.h"

#include "VulkanGraphicsDevice.h"
#include "VulkanContext.h"
#include "VulkanSwapchain.h"

namespace {

    void TransitionImageLayout(vk::raii::CommandBuffer& commandBuffer, const vk::raii::Image & image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t layerCount = 1);
    void CopyBufferToImage(vk::raii::CommandBuffer& commandBuffer, const vk::raii::Buffer& buffer, vk::raii::Image& image, uint32_t width, uint32_t height, const std::vector<vk::BufferImageCopy>& regions);
    void CopyBuffer(vk::raii::CommandBuffer& commandBuffer, const vk::raii::Buffer& srcBuffer, const vk::raii::Buffer& dstBuffer, vk::DeviceSize size);

    vk::raii::CommandBuffer BeginSingleTimeCommands(const vk::raii::Device& device, const vk::raii::CommandPool& commandPool);
    void EndSingleTimeCommands(vk::raii::CommandBuffer&& commandBuffer, const vk::raii::Queue queue);

}

namespace ZEngine {

	Scope<ResourceManager> ResourceManager::Create(const Scope<GraphicsDevice>& graphicsDevice) {
		return std::make_unique<VulkanResourceManager>(graphicsDevice);
	}

    VulkanResourceManager::VulkanResourceManager(const Scope<GraphicsDevice>& graphicsDevice) {
        m_GraphicsDevice = static_cast<VulkanGraphicsDevice*>(graphicsDevice.get());

        ImportExternalResources();
    }

    void VulkanResourceManager::ImportExternalResources() {
        auto& swapchain = m_GraphicsDevice->GetSwapchain();
        auto imageIndex = m_GraphicsDevice->GetCurrentImageIndex();

        m_ExternalPool[SWAPCHAIN_TEXTURE_ID] = ExternalTextureResource {
            .image = swapchain.GetImage(imageIndex),
            .imageView = swapchain.GetImageView(imageIndex),
            .memory = nullptr,
            .format = swapchain.GetSurfaceFormat().format,
            .width = swapchain.GetExtent().width,
            .height = swapchain.GetExtent().height,
        };

        m_ExternalPool[SWAPCHAIN_DEPTH_ID] = ExternalTextureResource{
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
        auto usage = ConvertBufferUsageFlags(info.usageFlags);

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
        VulkanBufferResource resource {
            .buffer = std::move(buffer),
            .memory = std::move(memory),
            .size = size,
            .mappedData = mappedPtr
        };
        return m_BufferPool.Allocate(std::move(resource));
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
                                        .mipLevels = info.levelCount,
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
            .subresourceRange = { .aspectMask = vk::ImageAspectFlagBits::eColor, .baseMipLevel = 0, .levelCount = info.levelCount, .baseArrayLayer = 0, .layerCount = 1 }
        };
        auto imageView = vk::raii::ImageView(context.GetDevice(), viewInfo);

        // Create texture resource for texture handle
        VulkanTextureResource resource {
            .image = std::move(image),
            .imageView = std::move(imageView),
            .memory = std::move(memory),
            .format = format,
            .width = width,
            .height = height
        };

        return m_TexturePool.Allocate(std::move(resource));
    }

    void VulkanResourceManager::UploadBufferData(BufferHandle dstHandle, const void* data, uint64_t size, BufferUsageFlags usage) {
        auto& context = m_GraphicsDevice->GetContext();
        auto& device = context.GetDevice();

        BufferCreateInfo stagingInfo {
            .size = size,
            .usageFlags = BufferUsageFlags::TransferSrc,
            .isHostVisible = true
        };

        auto stagingHandle = CreateBuffer(stagingInfo);
        auto stagingResource = ResolveBuffer(stagingHandle);

        void* stagingData = stagingResource->mappedData;
        memcpy(stagingData, data, size);
        stagingResource->memory.unmapMemory();

        auto dstResource = ResolveBuffer(dstHandle);

        vk::raii::CommandBuffer commandBuffer = BeginSingleTimeCommands(device, context.GetCurrentFrame().commandPool);
        CopyBuffer(commandBuffer, stagingResource->buffer, dstResource->buffer, size);
        EndSingleTimeCommands(std::move(commandBuffer), context.GetGraphicsQueue());

        DestroyBuffer(stagingHandle);
    }

    void VulkanResourceManager::DestroyBuffer(BufferHandle handle) {
        m_BufferPool.Free(handle);
    }

    void VulkanResourceManager::UploadTextureData(TextureHandle dstHandle,
                                                  const void* pixelData,
                                                  uint32_t width,
                                                  uint32_t height,
                                                  size_t size,
                                                  const std::vector<vk::BufferImageCopy>& regions)
    {
        auto& context = m_GraphicsDevice->GetContext();
        auto& device = context.GetDevice();

        // Staging buffer (Host visible)
        BufferCreateInfo stagingInfo{
            .size = size,
            .usageFlags = BufferUsageFlags::TransferSrc,
            .isHostVisible = true
        };

        auto stagingHandle = CreateBuffer(stagingInfo);
        auto stagingResource = ResolveBuffer(stagingHandle);

        void* stagingData = stagingResource->mappedData;
        memcpy(stagingData, pixelData, size);
        stagingResource->memory.unmapMemory();

        auto dstResource = ResolveTexture(dstHandle);

        vk::raii::CommandBuffer commandBuffer = BeginSingleTimeCommands(device, context.GetCurrentFrame().commandPool);
        TransitionImageLayout(commandBuffer, dstResource->image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, 1);
        CopyBufferToImage(commandBuffer, stagingResource->buffer, dstResource->image, static_cast<uint32_t>(width), static_cast<uint32_t>(height), regions);
        TransitionImageLayout(commandBuffer, dstResource->image, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, 1);
        EndSingleTimeCommands(std::move(commandBuffer), context.GetGraphicsQueue());

        DestroyBuffer(stagingHandle);
    }

    void VulkanResourceManager::DestroyTexture(TextureHandle handle) {
        m_TexturePool.Free(handle);
    }

    SamplerHandle VulkanResourceManager::CreateSampler(const SamplerCreateInfo& info) {
        auto& context = m_GraphicsDevice->GetContext();
        auto& device = context.GetDevice();
        auto& physicalDevice = context.GetPhysicalDevice();

        vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();
        vk::SamplerCreateInfo        samplerInfo{ .magFilter = info.magFilter,
                                                  .minFilter = info.minFilter,
                                                  .mipmapMode = info.mipmapMode,
                                                  .addressModeU = info.addressModeU,
                                                  .addressModeV = info.addressModeV,
                                                  .addressModeW = info.addressModeW,
                                                  .mipLodBias = info.mipLodBias,
                                                  .anisotropyEnable = info.enableAnisotropy,
                                                  .maxAnisotropy = info.maxAnisotropy,
                                                  .compareEnable = info.enableCompare,
                                                  .compareOp = info.compareOp,
                                                  .minLod = info.minLod,
                                                  .maxLod = info.maxLod };
        auto sampler = vk::raii::Sampler(device, samplerInfo);

        // Create sampler resource for sampler handle
        VulkanSamplerResource resource {
            .sampler = std::move(sampler)
        };

        return m_SamplerPool.Allocate(std::move(resource));
    }

    void VulkanResourceManager::DestroySampler(SamplerHandle handle) {
        m_SamplerPool.Free(handle);
    }

    VulkanBufferResource* VulkanResourceManager::ResolveBuffer(BufferHandle handle) {
        return m_BufferPool.Get(handle);
    }

    VulkanTextureResource* VulkanResourceManager::ResolveTexture(TextureHandle handle) {
        return m_TexturePool.Get(handle);
    }

    VulkanSamplerResource* VulkanResourceManager::ResolveSampler(SamplerHandle handle) {
        return m_SamplerPool.Get(handle);
    }

    ExternalTextureResource* VulkanResourceManager::ResolveExternal(TextureHandle handle) {
        assert(this != nullptr && "The containing class instance is NULL!");

        auto it = m_ExternalPool.find(handle.GetIndex());
        return (it != m_ExternalPool.end()) ? &it->second : nullptr;
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
            case TextureFormat::BC7_UNORM:         return vk::Format::eBc7UnormBlock;
            case TextureFormat::BC7_SRBG:          return vk::Format::eBc7SrgbBlock;
            default:                               return vk::Format::eR8G8B8A8Srgb;
        }
    }

    vk::BufferUsageFlags VulkanResourceManager::ConvertBufferUsageFlags(BufferUsageFlags flags) {
        vk::BufferUsageFlags vkFlags{};

        uint32_t rawFlags = static_cast<uint32_t>(flags);

        if (rawFlags & static_cast<uint32_t>(BufferUsageFlags::VertexBuffer)) {
            vkFlags |= vk::BufferUsageFlagBits::eVertexBuffer;
        }
        if (rawFlags & static_cast<uint32_t>(BufferUsageFlags::IndexBuffer)) {
            vkFlags |= vk::BufferUsageFlagBits::eIndexBuffer;
        }
        if (rawFlags & static_cast<uint32_t>(BufferUsageFlags::UniformBuffer)) {
            vkFlags |= vk::BufferUsageFlagBits::eUniformBuffer;
        }
        if (rawFlags & static_cast<uint32_t>(BufferUsageFlags::StorageBuffer)) {
            vkFlags |= vk::BufferUsageFlagBits::eStorageBuffer;
        }
        if (rawFlags & static_cast<uint32_t>(BufferUsageFlags::TransferSrc)) {
            vkFlags |= vk::BufferUsageFlagBits::eTransferSrc;
        }
        if (rawFlags & static_cast<uint32_t>(BufferUsageFlags::TransferDst)) {
            vkFlags |= vk::BufferUsageFlagBits::eTransferDst;
        }

        return vkFlags;
    }

    vk::ImageUsageFlags VulkanResourceManager::ConvertTextureUsageFlags(TextureUsageFlags flags) {
        vk::ImageUsageFlags vkFlags {};

        uint32_t rawFlags = static_cast<uint32_t>(flags);

        if (rawFlags & static_cast<uint32_t>(TextureUsageFlags::Sampled)) {
            vkFlags |= vk::ImageUsageFlagBits::eSampled;
        }
        if (rawFlags & static_cast<uint32_t>(TextureUsageFlags::ColorAttachment)) {
            vkFlags |= vk::ImageUsageFlagBits::eColorAttachment;
        }
        if (rawFlags & static_cast<uint32_t>(TextureUsageFlags::DepthAttachment)) {
            vkFlags |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
        }
        if (rawFlags & static_cast<uint32_t>(TextureUsageFlags::TransferSrc)) {
            vkFlags |= vk::ImageUsageFlagBits::eTransferSrc;
        }
        if (rawFlags & static_cast<uint32_t>(TextureUsageFlags::TransferDst)) {
            vkFlags |= vk::ImageUsageFlagBits::eTransferDst;
        }

        return vkFlags;
    }

}

namespace {

    void TransitionImageLayout(vk::raii::CommandBuffer& commandBuffer, const vk::raii::Image& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t layerCount) {
        vk::ImageMemoryBarrier barrier{ .oldLayout = oldLayout,
                                         .newLayout = newLayout,
                                         .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
                                         .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
                                         .image = image,
                                         .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .levelCount = 1, .layerCount = layerCount} };

        vk::PipelineStageFlags sourceStage;
        vk::PipelineStageFlags destinationStage;

        if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
            barrier.srcAccessMask = {};
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

            sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
            destinationStage = vk::PipelineStageFlagBits::eTransfer;
        }
        else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

            sourceStage = vk::PipelineStageFlagBits::eTransfer;
            destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
        }
        else {
            throw std::invalid_argument("unsupported layout transition!");
        }
        commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, {}, {}, barrier);
    }

    void CopyBufferToImage(vk::raii::CommandBuffer& commandBuffer, const vk::raii::Buffer& buffer, vk::raii::Image& image, uint32_t width, uint32_t height, const std::vector<vk::BufferImageCopy>& regions) {
        commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, regions);
    }

    void CopyBuffer(vk::raii::CommandBuffer& commandBuffer, const vk::raii::Buffer& srcBuffer, const vk::raii::Buffer& dstBuffer, vk::DeviceSize size) {
        vk::BufferCopy region{ .srcOffset = 0,
                               .dstOffset = 0,
                               .size = size };
        commandBuffer.copyBuffer(srcBuffer, dstBuffer, region);
    }

    vk::raii::CommandBuffer BeginSingleTimeCommands(const vk::raii::Device& device, const vk::raii::CommandPool& commandPool) {
        vk::CommandBufferAllocateInfo allocInfo{ .commandPool = commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1 };
        vk::raii::CommandBuffer       commandBuffer = std::move(vk::raii::CommandBuffers(device, allocInfo).front());

        vk::CommandBufferBeginInfo beginInfo{ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
        commandBuffer.begin(beginInfo);

        return commandBuffer;
    }

    void EndSingleTimeCommands(vk::raii::CommandBuffer&& commandBuffer, const vk::raii::Queue queue) {
        commandBuffer.end();

        vk::SubmitInfo submitInfo{ .commandBufferCount = 1, .pCommandBuffers = &*commandBuffer };
        queue.submit(submitInfo, nullptr);
        queue.waitIdle();
    }

}