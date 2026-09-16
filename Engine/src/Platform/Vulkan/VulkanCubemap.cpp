#include "VulkanCubemap.h"

#include <stb_image.h>

#include "ZEngine/Core/Application.h"
#include "VulkanContext.h"

#include "VulkanShader.h"
#include "Platform/Vulkan/VulkanBuffer.h"
#include "Platform/Vulkan/VulkanDescriptorAllocator.h"

namespace {

	vk::raii::ImageView CreateImageView(const vk::raii::Device& device, vk::Image const& image, vk::Format format, uint32_t levelCount);
	std::pair<vk::raii::Image, vk::raii::DeviceMemory> CreateImage(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, uint32_t width, uint32_t height, vk::Format format, uint32_t mipLevels, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties);
	void TransitionImageLayout(vk::raii::CommandBuffer& commandBuffer, const vk::raii::Image& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t layerCount);
	void CopyBufferToImage(vk::raii::CommandBuffer& commandBuffer, const vk::raii::Buffer& buffer, vk::raii::Image& image, uint32_t width, uint32_t height, const std::vector<vk::BufferImageCopy>& regions);
	std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> CreateBuffer(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);

	vk::raii::CommandBuffer BeginSingleTimeCommands(const vk::raii::Device& device, const vk::raii::CommandPool& commandPool);
	void EndSingleTimeCommands(vk::raii::CommandBuffer&& commandBuffer, const vk::raii::Queue queue);
	uint32_t FindMemoryType(vk::raii::PhysicalDevice physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties);

}

namespace ZEngine {

	Scope<Cubemap> Cubemap::Create() {
		return std::make_unique<VulkanCubemap>();
	}

    VulkanCubemap::VulkanCubemap() {
    }

	void VulkanCubemap::Init(const PipelineSpecification& spec) {
		CreatePipelineState(spec);
		AllocateDescriptorSet();
		UpdateDescriptorSet();
	}

    void VulkanCubemap::LoadCubemap(std::vector<std::string> faces) {
		auto vk_Context = static_cast<VulkanContext*>(Application::Get().GetGraphicsContext());
		auto& device = vk_Context->GetDevice();
		auto& physicalDevice = vk_Context->GetPhysicalDevice();
		auto& commandPool = vk_Context->GetCommandPool();
		auto queue = vk_Context->GetGraphicsQueue();

		//stbi_set_flip_vertically_on_load(true);

		int texWidth, texHeight, texChannels;
		stbi_uc* pixels[6] {};

		for (int i = 0; i < 6; ++i) {
			pixels[i] = stbi_load(faces[i].c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		}

		if (!pixels) {
			throw std::runtime_error("Failed to load cubemap images");
		}

		vk::DeviceSize layerSize = texWidth * texHeight * 4;
		vk::DeviceSize imageSize = layerSize * 6;

		std::tie(m_Image, m_ImageMemory) = CreateImage(device, physicalDevice, texWidth, texHeight,
			vk::Format::eR8G8B8A8Srgb,
			1,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
			vk::MemoryPropertyFlagBits::eDeviceLocal);

		auto [stagingBuffer, stagingBufferMemory] =
			CreateBuffer(device, physicalDevice, imageSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

		void* data = stagingBufferMemory.mapMemory(0, imageSize);
		for (int i = 0; i < 6; ++i) {
			uint8_t* dest = static_cast<uint8_t*>(data) + (i * layerSize);
			memcpy(dest, pixels[i], static_cast<size_t>(layerSize));
			stbi_image_free(pixels[i]);
		}
		stagingBufferMemory.unmapMemory();

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

		vk::raii::CommandBuffer commandBuffer = BeginSingleTimeCommands(device, commandPool);
		TransitionImageLayout(commandBuffer, m_Image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, 6);
		CopyBufferToImage(commandBuffer, stagingBuffer, m_Image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), { copyRegions });
		TransitionImageLayout(commandBuffer, m_Image, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, 6);
		EndSingleTimeCommands(std::move(commandBuffer), queue);

		CreateCubemapImageView(device, vk::Format::eR8G8B8A8Srgb, 1);
		CreateCubemapSampler(device, physicalDevice, 0.0f);
    }

	void VulkanCubemap::CreatePipelineState(const PipelineSpecification& spec) {
		auto vk_Context = static_cast<VulkanContext*>(Application::Get().GetGraphicsContext());
		auto vk_Shader = static_cast<VulkanShader*>(spec.Shader.get());
		auto& device = vk_Context->GetDevice();

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
		auto vk_LayoutManager = static_cast<VulkanLayoutManager*>(vk_Context->GetLayoutManager().get());
		m_PipelineLayout = vk_LayoutManager->GetGlobalPipelineLayout();

		vk::Format depthFormat = vk_Context->GetDepthFormat();
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
			.layout = m_PipelineLayout
		};

		m_Pipeline = vk::raii::Pipeline(device, nullptr, pipelineInfo);
	}

	void VulkanCubemap::CreateCubemapImageView(const vk::raii::Device& device, vk::Format format, uint32_t levelCount) {
		m_ImageView = CreateImageView(device, *m_Image, format, levelCount);
	}

	void VulkanCubemap::CreateCubemapSampler(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, float maxLod) {
		vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();
		vk::SamplerCreateInfo        samplerInfo{ .magFilter = vk::Filter::eLinear,
												  .minFilter = vk::Filter::eLinear,
												  .mipmapMode = vk::SamplerMipmapMode::eLinear,
												  .addressModeU = vk::SamplerAddressMode::eClampToEdge,
												  .addressModeV = vk::SamplerAddressMode::eClampToEdge,
												  .addressModeW = vk::SamplerAddressMode::eClampToEdge,
												  .mipLodBias = 0.0f,
												  .anisotropyEnable = vk::True,
												  .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
												  .compareEnable = vk::False,
												  .compareOp = vk::CompareOp::eAlways,
												  .minLod = 0.0f,
												  .maxLod = maxLod };
		m_Sampler = vk::raii::Sampler(device, samplerInfo);
	}

    void VulkanCubemap::AllocateDescriptorSet() {
		auto vk_Context = static_cast<VulkanContext*>(Application::Get().GetGraphicsContext());

		auto vk_Allocator = static_cast<VulkanDescriptorAllocator*>(vk_Context->GetDescriptorAllocator().get());
		m_DescriptorSet = vk_Allocator->Allocate(SetSlot::Skybox);
    }

	void VulkanCubemap::UpdateDescriptorSet() {
		auto vk_Context = static_cast<VulkanContext*>(Application::Get().GetGraphicsContext());
		auto& device = vk_Context->GetDevice();

		vk::DescriptorImageInfo imageInfo { .sampler = m_Sampler, .imageView = m_ImageView, .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal };
		vk::WriteDescriptorSet descriptorWrite { .dstSet = *m_DescriptorSet,
												 .dstBinding = 0, // Set 1, Binding 0 (Skybox sampler)
												 .dstArrayElement = 0,
												 .descriptorCount = 1,
												 .descriptorType = vk::DescriptorType::eCombinedImageSampler,
												 .pImageInfo = &imageInfo };

		device.updateDescriptorSets(descriptorWrite, {});
	}

}

namespace {

	// viewType is cube, layerCount = 6
	vk::raii::ImageView CreateImageView(const vk::raii::Device& device, vk::Image const& image, vk::Format format, uint32_t levelCount) {
		vk::ImageViewCreateInfo viewInfo{
			.image = image,
			.viewType = vk::ImageViewType::eCube,
			.format = format,
			.subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .baseMipLevel = 0, .levelCount = levelCount, .baseArrayLayer = 0, .layerCount = 6} };
		return vk::raii::ImageView(device, viewInfo);
	}

	// flags = vk::ImageCreateFlagBits::eCubeCompatible, arrayLayers = 6
	std::pair<vk::raii::Image, vk::raii::DeviceMemory> CreateImage(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, uint32_t width, uint32_t height, vk::Format format, uint32_t mipLevels, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties) {
		vk::ImageCreateInfo imageInfo{ .flags = vk::ImageCreateFlagBits::eCubeCompatible,
									   .imageType = vk::ImageType::e2D,
									   .format = format,
									   .extent = {width, height, 1},
									   .mipLevels = mipLevels,
									   .arrayLayers = 6,
									   .samples = vk::SampleCountFlagBits::e1,
									   .tiling = tiling,
									   .usage = usage,
									   .sharingMode = vk::SharingMode::eExclusive };

		vk::raii::Image image = vk::raii::Image(device, imageInfo);

		vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
		vk::MemoryAllocateInfo allocInfo{ .allocationSize = memRequirements.size,
										 .memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties) };
		vk::raii::DeviceMemory imageMemory = vk::raii::DeviceMemory(device, allocInfo);
		image.bindMemory(imageMemory, 0);

		return { std::move(image), std::move(imageMemory) };
	}

	// Takes in layerCount parameter
	void TransitionImageLayout(vk::raii::CommandBuffer& commandBuffer, const vk::raii::Image& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t layerCount) {
		vk::ImageMemoryBarrier barrier { .oldLayout = oldLayout,
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

	std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> CreateBuffer(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties) {
		vk::BufferCreateInfo   bufferInfo{ .size = size, .usage = usage, .sharingMode = vk::SharingMode::eExclusive };
		vk::raii::Buffer       buffer = vk::raii::Buffer(device, bufferInfo);
		vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();
		vk::MemoryAllocateInfo allocInfo{ .allocationSize = memRequirements.size, .memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties) };
		vk::raii::DeviceMemory bufferMemory = vk::raii::DeviceMemory(device, allocInfo);
		buffer.bindMemory(*bufferMemory, 0);
		return { std::move(buffer), std::move(bufferMemory) };
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

	uint32_t FindMemoryType(vk::raii::PhysicalDevice physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
		vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		throw std::runtime_error("failed to find suitable memory type!");
	}

}