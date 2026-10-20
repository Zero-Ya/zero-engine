#pragma once

#include "ZEngine/Renderer/Cubemap.h"

#include "ZEngine/RHI/RHICommon.h"
#include "ZEngine/RHI/RHITypes.h"

#include <vulkan/vulkan_raii.hpp>

namespace ZEngine {

	class VulkanGraphicsDevice;
	class VulkanResourceManager;
	class VulkanBindlessManager;

	class VulkanCubemap : public Cubemap {
	public:
		VulkanCubemap(const Scope<GraphicsDevice>& graphicsDevice, const Scope<BindlessDescriptorManager>& bindlessManager, const Scope<ResourceManager>& resourceManager);
		virtual ~VulkanCubemap() = default;

		virtual void Init(const PipelineSpecification& spec) override;

		virtual std::optional<CubemapData> LoadCubemap(std::vector<std::string> faces) override;

		const vk::raii::Pipeline& GetNativePipeline() const { return m_Pipeline; }

		const uint32_t GetTextureIndex() const { return m_TextureIndex; }
		const uint32_t GetSamplerIndex() const { return m_SamplerIndex; }

	private:
		void CreatePipelineState(const PipelineSpecification& spec);

		uint32_t RegisterTexture(TextureHandle handle, const TextureLayout& layout);
		uint32_t RegisterSampler();

	private:
		vk::raii::Pipeline m_Pipeline = nullptr;

		uint32_t m_TextureIndex;
		uint32_t m_SamplerIndex;

		VulkanGraphicsDevice*  m_GraphicsDevice  = nullptr;
		VulkanResourceManager* m_ResourceManager = nullptr;
		VulkanBindlessManager* m_BindlessManager = nullptr;
	};

}