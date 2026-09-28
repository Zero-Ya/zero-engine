#include "VulkanSwapchain.h"

#include "ZEngine/Core/Application.h"
#include "VulkanGraphicsDevice.h"
#include "VulkanContext.h"

namespace {

	uint32_t chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const& surfaceCapabilities);
	vk::SurfaceFormatKHR chooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& availableFormats);
	vk::PresentModeKHR chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const& availablePresentModes);
	vk::Extent2D chooseSwapExtent(vk::SurfaceCapabilitiesKHR const& capabilities, uint32_t width, uint32_t height);

}

namespace ZEngine {

	void VulkanSwapchain::Create(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, const vk::raii::SurfaceKHR& surface, uint32_t width, uint32_t height) {
		CreateSwapchain(device, physicalDevice, width, height);
		CreateImageViews(device, physicalDevice);
		CreateDepthResources(device, physicalDevice);
	}

	void VulkanSwapchain::Cleanup() {
		m_DepthImageView = nullptr;
		m_DepthImageMemory = nullptr;
		m_DepthImage = nullptr;
		m_ImageViews.clear();
		m_Images.clear();
		m_Swapchain = nullptr;
	}

	void VulkanSwapchain::Recreate(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, const vk::raii::SurfaceKHR& surface, uint32_t width, uint32_t height) {
		Cleanup();
		device.waitIdle();

		CreateSwapchain(device, physicalDevice, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
		CreateImageViews(device, physicalDevice);
		CreateDepthResources(device, physicalDevice);

		//vk::raii::SwapchainKHR oldSwapchain = std::move(m_Swapchain);

		//m_Extent = vk::Extent2D{ width, height };

		//auto surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);
		//uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
		//if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount) {
		//	imageCount = surfaceCapabilities.maxImageCount;
		//}

		//vk::SwapchainCreateInfoKHR createInfo {
		//	.surface = *surface,
		//	.minImageCount = imageCount,
		//	.imageFormat = m_SurfaceFormat.format,
		//	.imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear,
		//	.imageExtent = m_Extent,
		//	.imageArrayLayers = 1,
		//	.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst,
		//	.imageSharingMode = vk::SharingMode::eExclusive,
		//	.preTransform = surfaceCapabilities.currentTransform,
		//	.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
		//	.presentMode = vk::PresentModeKHR::eFifo,
		//	.clipped = VK_TRUE,
		//	.oldSwapchain = *oldSwapchain // Pass old handle for driver reuse
		//};

		//m_Swapchain = vk::raii::SwapchainKHR(device, createInfo);
		//m_Images = m_Swapchain.getImages();

		//for (auto& img : m_Images) {
		//	vk::ImageViewCreateInfo viewInfo{
		//		.image = img,
		//		.viewType = vk::ImageViewType::e2D,
		//		.format = m_SurfaceFormat.format,
		//		.subresourceRange = {
		//			.aspectMask = vk::ImageAspectFlagBits::eColor,
		//			.baseMipLevel = 0,
		//			.levelCount = 1,
		//			.baseArrayLayer = 0,
		//			.layerCount = 1
		//		}
		//	};
		//	m_ImageViews.emplace_back(device, viewInfo);
		//}

		//vk::ImageCreateInfo depthImageInfo {
		//	.imageType = vk::ImageType::e2D,
		//	.format = m_DepthFormat,
		//	.extent = { m_Extent.width, m_Extent.height, 1 },
		//	.mipLevels = 1,
		//	.arrayLayers = 1,
		//	.samples = vk::SampleCountFlagBits::e1,
		//	.tiling = vk::ImageTiling::eOptimal,
		//	.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
		//	.sharingMode = vk::SharingMode::eExclusive
		//};
		//m_DepthImage = vk::raii::Image(device, depthImageInfo);

		//vk::MemoryRequirements memReqs = m_DepthImage.getMemoryRequirements();
		//vk::MemoryAllocateInfo allocInfo{
		//	.allocationSize = memReqs.size,
		//	.memoryTypeIndex = FindMemoryType(physicalDevice, memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal)
		//};
		//m_DepthImageMemory = vk::raii::DeviceMemory(device, allocInfo);
		//m_DepthImage.bindMemory(*m_DepthImageMemory, 0);

		//vk::ImageViewCreateInfo depthViewInfo{
		//	.image = *m_DepthImage,
		//	.viewType = vk::ImageViewType::e2D,
		//	.format = m_DepthFormat,
		//	.subresourceRange = {
		//		.aspectMask = vk::ImageAspectFlagBits::eDepth,
		//		.baseMipLevel = 0,
		//		.levelCount = 1,
		//		.baseArrayLayer = 0,
		//		.layerCount = 1
		//	}
		//};
		//m_DepthImageView = vk::raii::ImageView(device, depthViewInfo);
	}

	void VulkanSwapchain::CreateSwapchain(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, uint32_t width, uint32_t height) {
		auto vk_GraphicsDevice = static_cast<VulkanGraphicsDevice*>(Application::Get().GetGraphicsDevice().get());
		auto& vk_Context = vk_GraphicsDevice->GetContext();
		auto& surface = vk_Context.GetSurface();

		vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);

		// Surface format
		std::vector<vk::SurfaceFormatKHR> availableFormats = physicalDevice.getSurfaceFormatsKHR(*surface);
		m_SurfaceFormat = chooseSwapSurfaceFormat(availableFormats);

		// Choose extent and get image count
		m_Extent = chooseSwapExtent(surfaceCapabilities, width, height);
		m_MinImageCount = chooseSwapMinImageCount(surfaceCapabilities);

		// Choose available present mode
		std::vector<vk::PresentModeKHR> availablePresentModes = physicalDevice.getSurfacePresentModesKHR(*surface);
		vk::PresentModeKHR presentMode = chooseSwapPresentMode(availablePresentModes);

		// Swapchain create info
		vk::SwapchainCreateInfoKHR swapChainCreateInfo{ .surface = *surface,
														.minImageCount = m_MinImageCount,
														.imageFormat = m_SurfaceFormat.format,
														.imageColorSpace = m_SurfaceFormat.colorSpace,
														.imageExtent = m_Extent,
														.imageArrayLayers = 1,
														.imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
														.imageSharingMode = vk::SharingMode::eExclusive,
														.preTransform = surfaceCapabilities.currentTransform,
														.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
														.presentMode = presentMode,
														.clipped = true };
		// Create swapchain
		m_Swapchain = vk::raii::SwapchainKHR(device, swapChainCreateInfo);
		m_Images = m_Swapchain.getImages();
	}

	void VulkanSwapchain::CreateImageViews(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice) {
		assert(m_ImageViews.empty());

		vk::ImageViewCreateInfo imageViewCreateInfo{ .viewType = vk::ImageViewType::e2D,
													 .format = m_SurfaceFormat.format,
													 .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1} };
		for (auto& image : m_Images) {
			imageViewCreateInfo.image = image;
			m_ImageViews.emplace_back(device, imageViewCreateInfo);
		}
	}

	void VulkanSwapchain::CreateDepthResources(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice) {
		m_DepthFormat = FindDepthFormat(physicalDevice);

		std::tie(m_DepthImage, m_DepthImageMemory) = CreateImage(device, physicalDevice, m_Extent.width, m_Extent.height, m_DepthFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal);
		m_DepthImageView = CreateImageView(device, m_DepthImage, m_DepthFormat, vk::ImageAspectFlagBits::eDepth);
	}


	vk::Format VulkanSwapchain::FindDepthFormat(const vk::raii::PhysicalDevice& physicalDevice) {
		std::vector<vk::Format> candidates = {
			vk::Format::eD32Sfloat,
			vk::Format::eD32SfloatS8Uint,
			vk::Format::eD24UnormS8Uint
		};

		for (vk::Format format : candidates) {
			vk::FormatProperties props = physicalDevice.getFormatProperties(format);
			if ((props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) == vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
				return format;
			}
		}
		throw std::runtime_error("Failed to find supported depth format!");
	}

	uint32_t VulkanSwapchain::FindMemoryType(const vk::raii::PhysicalDevice& physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
		auto memProperties = physicalDevice.getMemoryProperties();
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}
		throw std::runtime_error("Failed to find suitable memory type for depth image!");
	}

	std::pair<vk::raii::Image, vk::raii::DeviceMemory> VulkanSwapchain::CreateImage(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties) {
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

	vk::raii::ImageView VulkanSwapchain::CreateImageView(const vk::raii::Device& device, vk::Image const& image, vk::Format format, vk::ImageAspectFlags aspectFlags) {
		vk::ImageViewCreateInfo viewInfo{
			.image = image,
			.viewType = vk::ImageViewType::e2D,
			.format = format,
			.subresourceRange = {.aspectMask = aspectFlags, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1} };
		return vk::raii::ImageView(device, viewInfo);
	}

}

namespace {

	uint32_t chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const& surfaceCapabilities) {
		auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
		if ((0 < surfaceCapabilities.maxImageCount) && (surfaceCapabilities.maxImageCount < minImageCount)) {
			minImageCount = surfaceCapabilities.maxImageCount;
		}
		return minImageCount;
	}

	vk::SurfaceFormatKHR chooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& availableFormats) {
		assert(!availableFormats.empty());
		const auto formatIt = std::ranges::find_if(
			availableFormats,
			[](const auto& format) { return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear; });
		return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
	}

	vk::PresentModeKHR chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const& availablePresentModes) {
		assert(std::ranges::any_of(availablePresentModes, [](auto presentMode) { return presentMode == vk::PresentModeKHR::eFifo; }));
		return std::ranges::any_of(availablePresentModes,
			[](const vk::PresentModeKHR value) { return vk::PresentModeKHR::eMailbox == value; }) ?
			vk::PresentModeKHR::eMailbox :
			vk::PresentModeKHR::eFifo;
	}

	vk::Extent2D chooseSwapExtent(vk::SurfaceCapabilitiesKHR const& capabilities, uint32_t width, uint32_t height) {
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		}
		return {
			std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
			std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height) };
	}

}