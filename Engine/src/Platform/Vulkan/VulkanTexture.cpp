#include "VulkanTexture.h"

#include "ZEngine/Core/Application.h"
#include "VulkanContext.h"

#include <filesystem>

#include <stb_image.h>
#include <ktx.h>
#include <ktxvulkan.h>

#include "VulkanDescriptorAllocator.h"

namespace {

	vk::raii::ImageView CreateImageView(const vk::raii::Device& device, vk::Image const& image, vk::Format format, uint32_t mipLevels);
	std::pair<vk::raii::Image, vk::raii::DeviceMemory> CreateImage(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, uint32_t width, uint32_t height, vk::Format format, uint32_t mipLevels, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties);
	void TransitionImageLayout(vk::raii::CommandBuffer& commandBuffer, const vk::raii::Image& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
	void CopyBufferToImage(vk::raii::CommandBuffer& commandBuffer, const vk::raii::Buffer& buffer, vk::raii::Image& image, uint32_t width, uint32_t height, const std::vector<vk::BufferImageCopy>& regions);
	std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> CreateBuffer(const vk::raii::Device & device, const vk::raii::PhysicalDevice & physicalDevice, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);

	vk::raii::CommandBuffer BeginSingleTimeCommands(const vk::raii::Device & device, const vk::raii::CommandPool & commandPool);
	void EndSingleTimeCommands(vk::raii::CommandBuffer && commandBuffer, const vk::raii::Queue queue);
	uint32_t FindMemoryType(vk::raii::PhysicalDevice physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties);

}

namespace ZEngine {

	VulkanTexture2D::VulkanTexture2D()
		: m_Path(""), m_Width(0), m_Height(0), m_RendererID(0)
	{}

	void VulkanTexture2D::LoadTexture(const std::string& path) {
		auto vk_Context = static_cast<VulkanContext*>(Application::Get().GetGraphicsContext());
		auto& device = vk_Context->GetDevice();
		auto& physicalDevice = vk_Context->GetPhysicalDevice();
		auto& commandPool = vk_Context->GetCommandPool();
		auto queue = vk_Context->GetGraphicsQueue();

		m_Path = path;

		std::filesystem::path filePath = path;
		std::string ext = filePath.extension().string().c_str();
		// Transform extension to lowercase
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

		if (ext == ".ktx" || ext == ".ktx2") {
			return LoadKTXTexture(device, physicalDevice, commandPool, queue);
		}
		else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".bmp") {
			return LoadStandardImage(device, physicalDevice, commandPool, queue);
		}
		throw std::runtime_error("Unsupported texture format: " + ext);
	}

	void VulkanTexture2D::UpdateDescriptorSet(const vk::raii::DescriptorSet& descriptorSet) {
		auto vk_Context = static_cast<VulkanContext*>(Application::Get().GetGraphicsContext());
		auto& device = vk_Context->GetDevice();

        vk::DescriptorImageInfo imageInfo { .sampler = m_Sampler, .imageView = m_ImageView, .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal };
        vk::WriteDescriptorSet descriptorWrite { .dstSet = *descriptorSet,
                                                 .dstBinding = 1, // Set 2, Binding 1 (Sampler)
                                                 .dstArrayElement = 0,
                                                 .descriptorCount = 1,
                                                 .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                                                 .pImageInfo = &imageInfo };

        device.updateDescriptorSets(descriptorWrite, {});
	}

	// Create standard texture image
	void VulkanTexture2D::LoadStandardImage(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, const vk::raii::CommandPool& commandPool, const vk::raii::Queue queue) {
		stbi_set_flip_vertically_on_load(true);

		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load((m_Path).c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		vk::DeviceSize imageSize = texWidth * texHeight * 4;

		m_Width = texWidth;
		m_Height = texHeight;

		if (!pixels) {
			throw std::runtime_error("Failed to load standard image:" + m_Path);
		}

		std::tie(m_Image, m_ImageMemory) = CreateImage(device, physicalDevice, texWidth, texHeight,
			vk::Format::eR8G8B8A8Srgb,
			1,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
			vk::MemoryPropertyFlagBits::eDeviceLocal);

		auto [stagingBuffer, stagingBufferMemory] =
			CreateBuffer(device, physicalDevice, imageSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

		void* data = stagingBufferMemory.mapMemory(0, imageSize);
		memcpy(data, pixels, imageSize);
		stagingBufferMemory.unmapMemory();

		vk::BufferImageCopy region{ .bufferOffset = 0,
									.bufferRowLength = 0,
									.bufferImageHeight = 0,
									.imageSubresource = {.aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1},
									.imageOffset = {0, 0, 0},
									.imageExtent = {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1} };

		vk::raii::CommandBuffer commandBuffer = BeginSingleTimeCommands(device, commandPool);
		TransitionImageLayout(commandBuffer, m_Image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
		CopyBufferToImage(commandBuffer, stagingBuffer, m_Image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), { region });
		TransitionImageLayout(commandBuffer, m_Image, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
		EndSingleTimeCommands(std::move(commandBuffer), queue);

		stbi_image_free(pixels);

		CreateTextureImageView(device, vk::Format::eR8G8B8A8Srgb, 1);
		CreateTextureSampler(device, physicalDevice, 1);
	}

	// Create KTX texture image
	void VulkanTexture2D::LoadKTXTexture(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, const vk::raii::CommandPool& commandPool, const vk::raii::Queue queue) {
		ktxTexture* kTexture = nullptr;

		KTX_error_code result = ktxTexture_CreateFromNamedFile(m_Path.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &kTexture);

		if (result != KTX_SUCCESS) {
			throw std::runtime_error("Failed to load KTX texture: " + m_Path);
		}

		// Transcode Basis Universal KTX2 textures to BC7 if needed
		if (ktxTexture_NeedsTranscoding(kTexture)) {
			ktxTexture2* kTexture2 = reinterpret_cast<ktxTexture2*>(kTexture);
			ktxTexture2_TranscodeBasis(kTexture2, KTX_TTF_BC7_RGBA, 0);
		}

		// Get format, width, height and mip levels
		vk::Format format = static_cast<vk::Format>(ktxTexture_GetVkFormat(kTexture));
		uint32_t width = kTexture->baseWidth;
		uint32_t height = kTexture->baseHeight;
		uint32_t mipLevels = kTexture->numLevels;

		m_Width = width;
		m_Height = height;

		std::tie(m_Image, m_ImageMemory) = CreateImage(device, physicalDevice, width, height,
			format,
			mipLevels,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
			vk::MemoryPropertyFlagBits::eDeviceLocal);

		size_t ktxSize = ktxTexture_GetDataSize((ktxTexture*)kTexture);
		uint8_t* ktxData = ktxTexture_GetData((ktxTexture*)kTexture);
		auto [stagingBuffer, stagingBufferMemory] = CreateBuffer(device, physicalDevice, ktxSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

		void* data = stagingBufferMemory.mapMemory(0, ktxSize);
		memcpy(data, ktxData, ktxSize);
		stagingBufferMemory.unmapMemory();

		// Setup copy regions for all mip levels
		std::vector<vk::BufferImageCopy> copyRegions;
		for (uint32_t level = 0; level < mipLevels; ++level) {
			ktx_size_t offset = 0;
			ktxTexture_GetImageOffset((ktxTexture*)kTexture, level, 0, 0, &offset);

			vk::BufferImageCopy region{};
			region.bufferOffset = offset;
			region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
			region.imageSubresource.mipLevel = level;
			region.imageSubresource.layerCount = 1;
			region.imageExtent = vk::Extent3D{
				std::max(1u, width >> level),
				std::max(1u, height >> level),
				1
			};
			copyRegions.push_back(region);
		}

		vk::raii::CommandBuffer commandBuffer = BeginSingleTimeCommands(device, commandPool);
		TransitionImageLayout(commandBuffer, m_Image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
		CopyBufferToImage(commandBuffer, stagingBuffer, m_Image, static_cast<uint32_t>(width), static_cast<uint32_t>(height), copyRegions);
		TransitionImageLayout(commandBuffer, m_Image, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
		EndSingleTimeCommands(std::move(commandBuffer), queue);

		ktxTexture_Destroy((ktxTexture*)kTexture);

		CreateTextureImageView(device, format, mipLevels);
		CreateTextureSampler(device, physicalDevice, static_cast<float>(mipLevels));
	}

	void VulkanTexture2D::CreateTextureImageView(const vk::raii::Device& device, vk::Format format, uint32_t mipLevels) {
		m_ImageView = CreateImageView(device, *m_Image, format, mipLevels);
	}

	void VulkanTexture2D::CreateTextureSampler(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, float maxLod) {
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
												  .compareOp = vk::CompareOp::eAlways,
												  .minLod = 0.0f,
												  .maxLod = maxLod };
		m_Sampler = vk::raii::Sampler(device, samplerInfo);
	}

}

namespace {

	vk::raii::ImageView CreateImageView(const vk::raii::Device& device, vk::Image const& image, vk::Format format, uint32_t mipLevels) {
		vk::ImageViewCreateInfo viewInfo{
			.image = image,
			.viewType = vk::ImageViewType::e2D,
			.format = format,
			.subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .baseMipLevel = 0, .levelCount = mipLevels, .baseArrayLayer = 0, .layerCount = 1} };
		return vk::raii::ImageView(device, viewInfo);
	}

	std::pair<vk::raii::Image, vk::raii::DeviceMemory> CreateImage(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, uint32_t width, uint32_t height, vk::Format format, uint32_t mipLevels, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties) {
		vk::ImageCreateInfo imageInfo{ .imageType = vk::ImageType::e2D,
									   .format = format,
									   .extent = {width, height, 1},
									   .mipLevels = mipLevels,
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

	void TransitionImageLayout(vk::raii::CommandBuffer& commandBuffer, const vk::raii::Image& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout) {
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