#pragma once

#include "ZEngine/Renderer/Texture.h"

#include <vulkan/vulkan_raii.hpp>

namespace ZEngine {

	class VulkanTexture2D : public Texture2D {
	public:
		VulkanTexture2D(const std::string& path);
		virtual ~VulkanTexture2D() = default;

		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }

		virtual void Bind(uint32_t slot = 0) const override {};

		vk::ImageView GetImageView() const { return *m_ImageView; }
		vk::Sampler GetSampler() const { return *m_Sampler; }

	private:
		void CreateTextureImage(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, const vk::raii::CommandPool& commandPool, const vk::raii::Queue queue);
		void CreateTextureImageView(const vk::raii::Device& device);
		void CreateTextureSampler(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice);

		vk::raii::ImageView CreateImageView(const vk::raii::Device& device, vk::Image const& image, vk::Format format);
		std::pair<vk::raii::Image, vk::raii::DeviceMemory> CreateImage(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties);
		void TransitionImageLayout(vk::raii::CommandBuffer& commandBuffer, const vk::raii::Image& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
		void CopyBufferToImage(vk::raii::CommandBuffer& commandBuffer, const vk::raii::Buffer& buffer, vk::raii::Image& image, uint32_t width, uint32_t height);
		std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> CreateBuffer(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
		vk::raii::CommandBuffer BeginSingleTimeCommands(const vk::raii::Device& device, const vk::raii::CommandPool& commandPool);
		void EndSingleTimeCommands(vk::raii::CommandBuffer&& commandBuffer, const vk::raii::Queue queue);
		uint32_t FindMemoryType(vk::raii::PhysicalDevice physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties);

	private:
		std::string m_Path;
		uint32_t m_Width, m_Height;
		uint32_t m_RendererID;

		vk::raii::Image m_Image = nullptr;
		vk::raii::DeviceMemory m_ImageMemory = nullptr;
		vk::raii::ImageView m_ImageView = nullptr;
		vk::raii::Sampler m_Sampler = nullptr;
	};

}