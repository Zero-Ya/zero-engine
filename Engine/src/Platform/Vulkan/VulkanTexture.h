#pragma once

#include "ZEngine/Renderer/Texture.h"

#include <vulkan/vulkan_raii.hpp>

namespace ZEngine {

	class VulkanTexture2D : public Texture2D {
	public:
		VulkanTexture2D();
		virtual ~VulkanTexture2D() = default;

		virtual void LoadTexture(const std::string& path) override;

		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }

		virtual void Bind(uint32_t slot = 0) const override {};

		vk::ImageView GetImageView() const { return *m_ImageView; }
		vk::Sampler GetSampler() const { return *m_Sampler; }

		virtual void UpdateDescriptorSet(const vk::raii::DescriptorSet& descriptorSet);

	private:
		void LoadStandardImage(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, const vk::raii::CommandPool& commandPool, const vk::raii::Queue queue);
		void LoadKTXTexture(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, const vk::raii::CommandPool& commandPool, const vk::raii::Queue queue);

		void CreateTextureImageView(const vk::raii::Device& device, vk::Format format, uint32_t mipLevels);
		void CreateTextureSampler(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, float maxLod);

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