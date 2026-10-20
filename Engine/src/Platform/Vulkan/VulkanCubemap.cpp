#include "VulkanCubemap.h"

#include "VulkanGraphicsDevice.h"
#include "VulkanContext.h"
#include "VulkanSwapchain.h"

#include "VulkanShader.h"

#include "VulkanBindlessManager.h"
#include "VulkanResourceManager.h"

#include <stb_image.h>

namespace ZEngine {

	Scope<Cubemap> Cubemap::Create(const Scope<GraphicsDevice>& graphicsDevice, const Scope<BindlessDescriptorManager>& bindlessManager, const Scope<ResourceManager>& resourceManager) {
		return std::make_unique<VulkanCubemap>(graphicsDevice, bindlessManager, resourceManager);
	}

	VulkanCubemap::VulkanCubemap(const Scope<GraphicsDevice>& graphicsDevice, const Scope<BindlessDescriptorManager>& bindlessManager, const Scope<ResourceManager>& resourceManager) {
		m_GraphicsDevice = static_cast<VulkanGraphicsDevice*>(graphicsDevice.get());
		m_ResourceManager = static_cast<VulkanResourceManager*>(resourceManager.get());
		m_BindlessManager = static_cast<VulkanBindlessManager*>(bindlessManager.get());
	}

	void VulkanCubemap::Init(const PipelineSpecification& spec) {
		CreatePipelineState(spec);
	}

	std::optional<CubemapData> VulkanCubemap::LoadCubemap(std::vector<std::string> faces) {
		auto& context = m_GraphicsDevice->GetContext();
		auto& device = context.GetDevice();

		//stbi_set_flip_vertically_on_load(true);

		int texWidth, texHeight, texChannels;
		stbi_uc* pixels[6] {};

		for (int i = 0; i < 6; ++i)
			pixels[i] = stbi_load(faces[i].c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

		if (!pixels)
			throw std::runtime_error("Failed to load cubemap images");

		vk::DeviceSize layerSize = texWidth * texHeight * 4;
		vk::DeviceSize imageSize = layerSize * 6;

		TextureCreateInfo textureInfo {
			.width = static_cast<uint32_t>(texWidth),
			.height = static_cast<uint32_t>(texHeight),
			.format = TextureFormat::RGBA8_SRGB,
			.usageFlags = TextureUsageFlags::Sampled | TextureUsageFlags::TransferDst,
			.levelCount = 1
		};
		TextureHandle imageHandle = m_ResourceManager->CreateCubemap(textureInfo);

		// Setup 6 copy regions (1 per face)
		std::vector<vk::BufferImageCopy> copyRegions;
		for (uint32_t face = 0; face < 6; ++face) {
			vk::BufferImageCopy copyRegion{};
			copyRegion.bufferOffset = face * layerSize;
			copyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
			copyRegion.imageSubresource.mipLevel = 0;
			copyRegion.imageSubresource.baseArrayLayer = face; // Specific layer
			copyRegion.imageSubresource.layerCount = 1;
			copyRegion.imageExtent = { static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1 };

			copyRegions.push_back(copyRegion);
		}

		m_ResourceManager->UploadCubemapData(imageHandle, pixels, texWidth, texHeight, layerSize, imageSize, copyRegions);

		auto textureIndex = RegisterTexture(imageHandle, TextureLayout::SHADER_READ);
		auto samplerIndex = RegisterSampler();

		m_TextureIndex = textureIndex;
		m_SamplerIndex = samplerIndex;

		CubemapData cubemap{};
		cubemap.textureIndex = textureIndex;
		cubemap.samplerIndex = samplerIndex;

		return cubemap;
    }

	void VulkanCubemap::CreatePipelineState(const PipelineSpecification& spec) {
		auto& context = m_GraphicsDevice->GetContext();
		auto& swapchain = m_GraphicsDevice->GetSwapchain();

		auto vk_Shader = static_cast<VulkanShader*>(spec.Shader.get());
		auto& device = context.GetDevice();

		// Shader stages info
		std::vector<vk::PipelineShaderStageCreateInfo> shaderStages = vk_Shader->GetShaderStages();

		// Vertex input info (None)
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};

		// Rasterizer info
		vk::PipelineRasterizationStateCreateInfo rasterizerInfo { .depthClampEnable = vk::False,
																  .rasterizerDiscardEnable = vk::False,
																  .polygonMode = vk::PolygonMode::eFill,
																  .cullMode = vk::CullModeFlagBits::eNone,
																  .frontFace = vk::FrontFace::eCounterClockwise,
																  .depthBiasEnable = vk::False,
																  .lineWidth = 1.0f };
		// Multisampling and color blending
		vk::PipelineMultisampleStateCreateInfo multisampling{ .rasterizationSamples = vk::SampleCountFlagBits::e1, .sampleShadingEnable = vk::False };

		// Depth stencil (depthWriteEnable = false, depthCompareOp = less or equal)
		vk::PipelineDepthStencilStateCreateInfo depthStencil{
			.depthTestEnable = vk::True,
			.depthWriteEnable = vk::False,
			.depthCompareOp = vk::CompareOp::eLessOrEqual,
			.depthBoundsTestEnable = vk::False,
			.stencilTestEnable = vk::False
		};

		// (blendEnable = false)
		vk::PipelineColorBlendAttachmentState  colorBlendAttachment{
			.blendEnable = vk::False,
			// RGB blending
			.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
			.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
			.colorBlendOp = vk::BlendOp::eAdd,

			// Alpha blending
			.srcAlphaBlendFactor = vk::BlendFactor::eOne,
			.dstAlphaBlendFactor = vk::BlendFactor::eZero,
			.alphaBlendOp = vk::BlendOp::eAdd,

			.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
		};

		vk::PipelineColorBlendStateCreateInfo  colorBlending{
			.logicOpEnable = vk::False, .logicOp = vk::LogicOp::eCopy, .attachmentCount = 1, .pAttachments = &colorBlendAttachment };

		// Input assembly and viewport state (primitiveRestartEnable = false)
		vk::PipelineInputAssemblyStateCreateInfo inputAssembly{ .topology = vk::PrimitiveTopology::eTriangleList, .primitiveRestartEnable = vk::False };
		vk::PipelineViewportStateCreateInfo		 viewportState{ .viewportCount = 1, .scissorCount = 1 };

		// Dynamic state info
		std::vector<vk::DynamicState>	   dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
		vk::PipelineDynamicStateCreateInfo dynamicState{ .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()), .pDynamicStates = dynamicStates.data() };

		// Pipeline layout info
		auto pipelineLayout = m_BindlessManager->GetPipelineLayout();

		vk::Format depthFormat = swapchain.GetDepthFormat();
		vk::Format colorFormat = vk::Format::eB8G8R8A8Srgb; // We can also get swapchain surface format
		vk::PipelineRenderingCreateInfo dynamicRenderingInfo{ .colorAttachmentCount = 1, .pColorAttachmentFormats = &colorFormat, .depthAttachmentFormat = depthFormat };

		// Pipeline info
		vk::GraphicsPipelineCreateInfo pipelineInfo{
			.pNext = &dynamicRenderingInfo,
			.stageCount = 2,
			.pStages = shaderStages.data(),
			.pVertexInputState = &vertexInputInfo,
			.pInputAssemblyState = &inputAssembly,
			.pViewportState = &viewportState,
			.pRasterizationState = &rasterizerInfo,
			.pMultisampleState = &multisampling,
			.pDepthStencilState = &depthStencil,
			.pColorBlendState = &colorBlending,
			.pDynamicState = &dynamicState,
			.layout = pipelineLayout
		};

		m_Pipeline = vk::raii::Pipeline(device, nullptr, pipelineInfo);
	}

	uint32_t VulkanCubemap::RegisterTexture(TextureHandle handle, const TextureLayout& layout) {
		auto bindlessIndex = m_BindlessManager->RegisterTexture(handle, layout);
		return bindlessIndex;
	}

	uint32_t VulkanCubemap::RegisterSampler() {
		auto& context = m_GraphicsDevice->GetContext();
		auto& physicalDevice = context.GetPhysicalDevice();

		// Create
		vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();
		SamplerHandle samplerHandle = m_ResourceManager->CreateSampler({
			.magFilter = vk::Filter::eLinear,
			.minFilter = vk::Filter::eLinear,
			.mipmapMode = vk::SamplerMipmapMode::eLinear,
			.addressModeU = vk::SamplerAddressMode::eClampToEdge,
			.addressModeV = vk::SamplerAddressMode::eClampToEdge,
			.addressModeW = vk::SamplerAddressMode::eClampToEdge,
			.mipLodBias = 0.0f,
			.enableAnisotropy = vk::True,
			.maxAnisotropy = properties.limits.maxSamplerAnisotropy,
			.enableCompare = vk::False,
			.compareOp = vk::CompareOp::eAlways,
			.minLod = 0.0f,
			.maxLod = 0.0f
		});

		// Register
		auto bindlessIndex = m_BindlessManager->RegisterSampler(samplerHandle);
		return bindlessIndex;
	}

}