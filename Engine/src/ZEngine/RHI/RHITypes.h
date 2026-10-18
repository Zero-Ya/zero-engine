#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace ZEngine {

    //struct BufferCreateInfo {
    //    size_t size = 0;
    //    vk::BufferUsageFlags usage;
    //    vk::MemoryPropertyFlags memoryProperties;
    //    bool debugMapped = false;
    //};

    //struct TextureCreateInfo {
    //    Extent2D extent;
    //    uint32_t mipLevels = 1;
    //    vk::Format format = vk::Format::eR8G8B8A8Unorm;
    //    vk::ImageUsageFlags usage;
    //    vk::MemoryPropertyFlags memoryProperties;
    //};

    struct SamplerCreateInfo {
        vk::Filter magFilter = vk::Filter::eLinear;
        vk::Filter minFilter = vk::Filter::eLinear;
        vk::SamplerMipmapMode mipmapMode = vk::SamplerMipmapMode::eLinear;
        vk::SamplerAddressMode addressModeU = vk::SamplerAddressMode::eRepeat;
        vk::SamplerAddressMode addressModeV = vk::SamplerAddressMode::eRepeat;
        vk::SamplerAddressMode addressModeW = vk::SamplerAddressMode::eRepeat;
        float mipLodBias = 0.0f;
        bool enableAnisotropy = true;
        float maxAnisotropy = 16.0f;
        bool enableCompare = false;
        vk::CompareOp compareOp = vk::CompareOp::eAlways;
        float minLod = 0.0f;
        float maxLod = VK_LOD_CLAMP_NONE;
        vk::BorderColor borderColor = vk::BorderColor::eFloatOpaqueBlack;
    };

}