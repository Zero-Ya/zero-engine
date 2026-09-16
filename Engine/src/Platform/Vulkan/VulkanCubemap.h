#pragma once

#include "ZEngine/Renderer/Cubemap.h"

#include "ZEngine/Renderer/PipelineState.h"

#include <vulkan/vulkan_raii.hpp>

namespace ZEngine {
	
	class Texture2D;
	class UniformBuffer;

	class VulkanCubemap : public Cubemap {
	public:
		VulkanCubemap();
		virtual ~VulkanCubemap() = default;

		virtual void Init(const PipelineSpecification& spec) override;

		virtual void LoadCubemap(std::vector<std::string> faces) override;

		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }

		const vk::raii::Pipeline& GetNativePipeline() const { return m_Pipeline; }
		const vk::PipelineLayout& GetRawNativeLayout() const { return m_PipelineLayout; }

		void UpdateDescriptorSet();

		const vk::raii::DescriptorSet& GetDescriptorSet() const { return m_DescriptorSet; }

	private:
		void CreatePipelineState(const PipelineSpecification& spec);

		void CreateCubemapImageView(const vk::raii::Device& device, vk::Format format, uint32_t levelCount);
		void CreateCubemapSampler(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, float maxLod);

		void AllocateDescriptorSet();

	private:
		uint32_t m_Width, m_Height;

		vk::PipelineLayout m_PipelineLayout = nullptr;
		vk::raii::Pipeline m_Pipeline = nullptr;

		vk::raii::DescriptorSet m_DescriptorSet = nullptr;

		vk::raii::Image m_Image = nullptr;
		vk::raii::DeviceMemory m_ImageMemory = nullptr;
		vk::raii::ImageView m_ImageView = nullptr;
		vk::raii::Sampler m_Sampler = nullptr;
	};

}