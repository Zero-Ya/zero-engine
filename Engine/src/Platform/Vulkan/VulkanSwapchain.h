#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace ZEngine {

	class VulkanSwapchain {
	public:
		VulkanSwapchain(uint32_t width, uint32_t height);
		~VulkanSwapchain() = default;

		vk::raii::SwapchainKHR& GetSwapchain() { return m_Swapchain; };
		vk::SurfaceFormatKHR GetSurfaceFormat() const { return m_SwapchainSurfaceFormat; }
		vk::Extent2D GetExtent() const { return m_Extent; };
		uint32_t GetImageCount() const { return static_cast<uint32_t>(m_ImageViews.size()); }

		const vk::raii::ImageView& GetImageView(uint32_t index) const { return m_ImageViews[index]; }
		const vk::Image& GetImage(uint32_t index) const { return m_SwapchainImages[index]; }
		const std::vector<vk::Image>& GetImages() const { return m_SwapchainImages; }
		const uint32_t GetMinImageCount() const { return minImageCount; }

		void CreateSwapchain(uint32_t width, uint32_t height);
		void CreateImageViews();
		void CleanupSwapchain();

	private:

		vk::raii::SwapchainKHR				 m_Swapchain = nullptr;
		vk::SurfaceFormatKHR				 m_SwapchainSurfaceFormat;
		vk::Extent2D						 m_Extent;

		std::vector<vk::Image>				 m_SwapchainImages;
		std::vector<vk::raii::ImageView>	 m_ImageViews;

		uint32_t							 minImageCount;
	};

}