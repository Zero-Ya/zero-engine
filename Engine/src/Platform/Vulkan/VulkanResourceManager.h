#pragma once

#include "ZEngine/Renderer/ResourceManager.h"

#include <vulkan/vulkan_raii.hpp>

namespace ZEngine {

    class VulkanGraphicsDevice;

	struct VulkanBufferResource {
        vk::Buffer buffer       = nullptr;
        vk::DeviceMemory memory = nullptr;
        uint64_t size           = 0;
        void* mappedData        = nullptr;
    };

    struct VulkanTextureResource {
        vk::Image image         = nullptr;
        vk::ImageView imageView = nullptr;
        vk::DeviceMemory memory = nullptr;
        vk::Format format       = vk::Format::eUndefined;
        uint32_t width          = 0;
        uint32_t height         = 0;
    };
    
    class VulkanResourceManager : public ResourceManager {

    public:
        VulkanResourceManager(const Scope<GraphicsDevice>& graphicsDevice);
        ~VulkanResourceManager() = default;

        void InitRenderingHandles() override;

        BufferHandle CreateBuffer(const BufferCreateInfo& info);
        void DestroyBuffer(BufferHandle handle);

        TextureHandle CreateTexture(const TextureCreateInfo& info);
        void DestroyTexture(TextureHandle handle);

        // Mainly for vulkan command list
        VulkanBufferResource* ResolveBuffer(BufferHandle handle);
        VulkanTextureResource* ResolveTexture(TextureHandle handle);

    private:
        uint32_t FindMemoryType(uint32_t typeFilter, uint32_t propertyFlags) const;

        VulkanGraphicsDevice* m_GraphicsDevice;

        vk::Format ConvertTextureFormat(TextureFormat format);
        vk::ImageUsageFlagBits ConvertTextureUsageFlags(TextureUsageFlags flag);

        uint64_t m_NextBufferId = 1;
        uint64_t m_NextTextureId = 1;
        std::unordered_map<uint64_t, VulkanBufferResource> m_Buffers;
        std::unordered_map<uint64_t, VulkanTextureResource> m_Textures;
    };

}