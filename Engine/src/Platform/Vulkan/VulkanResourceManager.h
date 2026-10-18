#pragma once

#include "ZEngine/Renderer/ResourceManager.h"

#include <vulkan/vulkan_raii.hpp>

#include "ZEngine/RHI/RHITypes.h"
#include "ZEngine/Core/GenerationalPool.h"

namespace ZEngine {

    class VulkanGraphicsDevice;

	struct VulkanBufferResource {
        vk::raii::Buffer buffer       = nullptr;
        vk::raii::DeviceMemory memory = nullptr;
        uint64_t size                 = 0;
        void* mappedData              = nullptr;
    };

    struct VulkanTextureResource {
        vk::raii::Image image         = nullptr;
        vk::raii::ImageView imageView = nullptr;

        vk::raii::DeviceMemory memory = nullptr;
        vk::Format format             = vk::Format::eUndefined;
        uint32_t width                = 0;
        uint32_t height               = 0;
    };

    struct VulkanSamplerResource {
        vk::raii::Sampler sampler     = nullptr;
    };

    // // // // //
    struct ExternalTextureResource {
        vk::Image image = nullptr;
        vk::ImageView imageView = nullptr;

        vk::raii::DeviceMemory memory = nullptr;
        vk::Format format = vk::Format::eUndefined;
        uint32_t width = 0;
        uint32_t height = 0;
    };
    
    class VulkanResourceManager : public ResourceManager {
    public:
        VulkanResourceManager(const Scope<GraphicsDevice>& graphicsDevice);
        ~VulkanResourceManager() = default;

        void ImportExternalResources() override;

        BufferHandle CreateBuffer(const BufferCreateInfo& info) override;
        void UploadBufferData(BufferHandle dstHandle, const void* data, uint64_t size, BufferUsageFlags usage);
        void DestroyBuffer(BufferHandle handle) override;

        TextureHandle CreateTexture(const TextureCreateInfo& info) override;
        void UploadTextureData(TextureHandle dstHandle,
                               const void* pixelData,
                               uint32_t width,
                               uint32_t height,
                               size_t size,
                               const std::vector<vk::BufferImageCopy>& regions);
        void DestroyTexture(TextureHandle handle) override;

        SamplerHandle CreateSampler(const SamplerCreateInfo& info);
        void DestroySampler(SamplerHandle handle);

        // Mainly for vulkan command list
        VulkanBufferResource* ResolveBuffer(BufferHandle handle);
        VulkanTextureResource* ResolveTexture(TextureHandle handle);
        VulkanSamplerResource* ResolveSampler(SamplerHandle handle);

        ExternalTextureResource* ResolveExternal(TextureHandle handle);

    private:
        uint32_t FindMemoryType(uint32_t typeFilter, uint32_t propertyFlags) const;

        VulkanGraphicsDevice* m_GraphicsDevice;

        vk::Format ConvertTextureFormat(TextureFormat format);
        vk::BufferUsageFlags ConvertBufferUsageFlags(BufferUsageFlags flags);
        vk::ImageUsageFlags ConvertTextureUsageFlags(TextureUsageFlags flags);

        std::unordered_map<uint32_t, ExternalTextureResource> m_ExternalPool;

        GenerationalPool<TextureHandle, VulkanTextureResource> m_TexturePool;
        GenerationalPool<BufferHandle, VulkanBufferResource> m_BufferPool;
        GenerationalPool<SamplerHandle, VulkanSamplerResource> m_SamplerPool;
    };

}