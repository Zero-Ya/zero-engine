#include "VulkanTexture.h"

#include "ZEngine/Core/Application.h"
#include "VulkanContext.h"

#include <stb_image.h>

namespace ZEngine {

	VulkanTexture2D::VulkanTexture2D(const std::string& path)
		: m_Path(path)
	{
		auto vk_Context = static_cast<VulkanContext*>(Application::Get().GetGraphicsContext());
		auto& device = vk_Context->GetDevice();
		auto& physicalDevice = vk_Context->GetPhysicalDevice();
		auto& commandPool = vk_Context->GetCommandPool();
		auto queue = vk_Context->GetGraphicsQueue();

		CreateTextureImage(device, physicalDevice, commandPool, queue);
		CreateTextureImageView(device);
		CreateTextureSampler(device, physicalDevice);
	}

	void VulkanTexture2D::CreateTextureImage(const vk::raii::Device& device,
											 const vk::raii::PhysicalDevice& physicalDevice,
											 const vk::raii::CommandPool& commandPool,
											 const vk::raii::Queue queue)
	{
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load((ASSETS_DIR"/textures/" + m_Path).c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		vk::DeviceSize imageSize = texWidth * texHeight * 4;

		if (!pixels) {
			throw std::runtime_error("failed to load texture image!");
		}

		auto [stagingBuffer, stagingBufferMemory] =
			CreateBuffer(device, physicalDevice, imageSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

		void* data = stagingBufferMemory.mapMemory(0, imageSize);
		memcpy(data, pixels, imageSize);
		stagingBufferMemory.unmapMemory();

		stbi_image_free(pixels);

		std::tie(m_Image, m_ImageMemory) = CreateImage(device, physicalDevice, texWidth,
			texHeight,
			vk::Format::eR8G8B8A8Srgb,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
			vk::MemoryPropertyFlagBits::eDeviceLocal);

		vk::raii::CommandBuffer commandBuffer = BeginSingleTimeCommands(device, commandPool);
		TransitionImageLayout(commandBuffer, m_Image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
		CopyBufferToImage(commandBuffer, stagingBuffer, m_Image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
		TransitionImageLayout(commandBuffer, m_Image, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
		EndSingleTimeCommands(std::move(commandBuffer), queue);
	}

	void VulkanTexture2D::CreateTextureImageView(const vk::raii::Device& device) {
		m_ImageView = CreateImageView(device, *m_Image, vk::Format::eR8G8B8A8Srgb);
	}

	void VulkanTexture2D::CreateTextureSampler(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice) {
		vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();
		vk::SamplerCreateInfo        samplerInfo{ .magFilter = vk::Filter::eLinear,
												 .minFilter = vk::Filter::eLinear,
												 .mipmapMode = vk::SamplerMipmapMode::eLinear,
												 .addressModeU = vk::SamplerAddressMode::eRepeat,
												 .addressModeV = vk::SamplerAddressMode::eRepeat,
												 .addressModeW = vk::SamplerAddressMode::eRepeat,
												 .mipLodBias = 0.0f,
												 .anisotropyEnable = vk::True,
												 .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
												 .compareEnable = vk::False,
												 .compareOp = vk::CompareOp::eAlways };
		m_Sampler = vk::raii::Sampler(device, samplerInfo);
	}

	vk::raii::ImageView VulkanTexture2D::CreateImageView(const vk::raii::Device& device, vk::Image const& image, vk::Format format) {
		vk::ImageViewCreateInfo viewInfo{
			.image = image,
			.viewType = vk::ImageViewType::e2D,
			.format = format,
			.subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1} };
		return vk::raii::ImageView(device, viewInfo);
	}

	std::pair<vk::raii::Image, vk::raii::DeviceMemory> VulkanTexture2D::CreateImage(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties) {
		vk::ImageCreateInfo imageInfo{ .imageType = vk::ImageType::e2D,
									  .format = format,
									  .extent = {width, height, 1},
									  .mipLevels = 1,
									  .arrayLayers = 1,
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

	void VulkanTexture2D::TransitionImageLayout(vk::raii::CommandBuffer& commandBuffer, const vk::raii::Image& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout) {
		vk::ImageMemoryBarrier barrier{ .oldLayout = oldLayout,
									   .newLayout = newLayout,
									   .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
									   .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
									   .image = image,
									   .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .levelCount = 1, .layerCount = 1} };

		vk::PipelineStageFlags sourceStage;
		vk::PipelineStageFlags destinationStage;

		if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
		{
			barrier.srcAccessMask = {};
			barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

			sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
			destinationStage = vk::PipelineStageFlagBits::eTransfer;
		}
		else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
		{
			barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

			sourceStage = vk::PipelineStageFlagBits::eTransfer;
			destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
		}
		else
		{
			throw std::invalid_argument("unsupported layout transition!");
		}
		commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, {}, {}, barrier);
	}

	void VulkanTexture2D::CopyBufferToImage(vk::raii::CommandBuffer& commandBuffer, const vk::raii::Buffer& buffer, vk::raii::Image& image, uint32_t width, uint32_t height) {
		vk::BufferImageCopy region{ .bufferOffset = 0,
								   .bufferRowLength = 0,
								   .bufferImageHeight = 0,
								   .imageSubresource = {.aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1},
								   .imageOffset = {0, 0, 0},
								   .imageExtent = {width, height, 1} };
		commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);
	}

	std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> VulkanTexture2D::CreateBuffer(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties) {
		vk::BufferCreateInfo   bufferInfo{ .size = size, .usage = usage, .sharingMode = vk::SharingMode::eExclusive };
		vk::raii::Buffer       buffer = vk::raii::Buffer(device, bufferInfo);
		vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();
		vk::MemoryAllocateInfo allocInfo{ .allocationSize = memRequirements.size, .memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties) };
		vk::raii::DeviceMemory bufferMemory = vk::raii::DeviceMemory(device, allocInfo);
		buffer.bindMemory(*bufferMemory, 0);
		return { std::move(buffer), std::move(bufferMemory) };
	}

	vk::raii::CommandBuffer VulkanTexture2D::BeginSingleTimeCommands(const vk::raii::Device& device, const vk::raii::CommandPool& commandPool) {
		vk::CommandBufferAllocateInfo allocInfo{ .commandPool = commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1 };
		vk::raii::CommandBuffer       commandBuffer = std::move(vk::raii::CommandBuffers(device, allocInfo).front());

		vk::CommandBufferBeginInfo beginInfo{ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
		commandBuffer.begin(beginInfo);

		return commandBuffer;
	}

	void VulkanTexture2D::EndSingleTimeCommands(vk::raii::CommandBuffer&& commandBuffer, const vk::raii::Queue queue) {
		commandBuffer.end();

		vk::SubmitInfo submitInfo{ .commandBufferCount = 1, .pCommandBuffers = &*commandBuffer };
		queue.submit(submitInfo, nullptr);
		queue.waitIdle();
	}

	uint32_t VulkanTexture2D::FindMemoryType(vk::raii::PhysicalDevice physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
		vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}

		throw std::runtime_error("failed to find suitable memory type!");
	}

}