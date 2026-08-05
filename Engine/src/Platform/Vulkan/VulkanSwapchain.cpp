#include "VulkanSwapchain.h"
#include "ZEngine/Core/Application.h"
#include "VulkanContext.h"

namespace {

	uint32_t chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const& surfaceCapabilities);
	vk::SurfaceFormatKHR chooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& availableFormats);
	vk::PresentModeKHR chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const& availablePresentModes);
	vk::Extent2D chooseSwapExtent(vk::SurfaceCapabilitiesKHR const& capabilities, uint32_t width, uint32_t height);

}

namespace ZEngine {

	VulkanSwapchain::VulkanSwapchain(uint32_t width, uint32_t height) 
		: m_Swapchain(nullptr) {
		CreateSwapchain(width, height);
		CreateImageViews();
	}

	void VulkanSwapchain::CreateSwapchain(uint32_t width, uint32_t height) {
		auto vk_Context = static_cast<VulkanContext*>(Application::Get().GetGraphicsContext());
		auto& device = vk_Context->GetDevice();
		auto& physicalDevice = vk_Context->GetPhysicalDevice();
		auto& surface = vk_Context->GetSurface();

		vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);

		// Surface format
		std::vector<vk::SurfaceFormatKHR> availableFormats = physicalDevice.getSurfaceFormatsKHR(*surface);
		m_SwapchainSurfaceFormat = chooseSwapSurfaceFormat(availableFormats);

		// Choose extent and get image count
		m_Extent = chooseSwapExtent(surfaceCapabilities, width, height);
		minImageCount = chooseSwapMinImageCount(surfaceCapabilities);

		// Choose available present mode
		std::vector<vk::PresentModeKHR> availablePresentModes = physicalDevice.getSurfacePresentModesKHR(*surface);
		vk::PresentModeKHR presentMode = chooseSwapPresentMode(availablePresentModes);

		// Swapchain create info
		vk::SwapchainCreateInfoKHR swapChainCreateInfo{ .surface = *surface,
														.minImageCount = minImageCount,
														.imageFormat = m_SwapchainSurfaceFormat.format,
														.imageColorSpace = m_SwapchainSurfaceFormat.colorSpace,
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
		m_SwapchainImages = m_Swapchain.getImages();
	}

	void VulkanSwapchain::CreateImageViews() {
		assert(m_ImageViews.empty());

		auto vk_Context = static_cast<VulkanContext*>(Application::Get().GetGraphicsContext());
		auto& device = vk_Context->GetDevice();

		vk::ImageViewCreateInfo imageViewCreateInfo{ .viewType = vk::ImageViewType::e2D,
													 .format = m_SwapchainSurfaceFormat.format,
													 .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1} };
		for (auto& image : m_SwapchainImages) {
			imageViewCreateInfo.image = image;
			m_ImageViews.emplace_back(device, imageViewCreateInfo);
		}
	}

	void VulkanSwapchain::CleanupSwapchain() {
		m_ImageViews.clear();
		m_Swapchain = nullptr;
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