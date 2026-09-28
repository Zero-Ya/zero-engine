#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace ZEngine {

	class VulkanSwapchain {
	public:
		VulkanSwapchain() = default;
		~VulkanSwapchain() = default;

		void Create(const vk::raii::Device& device,
					const vk::raii::PhysicalDevice& physicalDevice,
					const vk::raii::SurfaceKHR& surface,
					uint32_t width, uint32_t height);

		void Cleanup();

		void Recreate(const vk::raii::Device& device,
					  const vk::raii::PhysicalDevice& physicalDevice,
					  const vk::raii::SurfaceKHR& surface,
					  uint32_t width, uint32_t height);

		vk::raii::SwapchainKHR& GetHandle() { return m_Swapchain; };
		vk::SurfaceFormatKHR GetSurfaceFormat() const { return m_SurfaceFormat; }
		vk::Extent2D GetExtent() const { return m_Extent; };
		uint32_t GetImageCount() const { return static_cast<uint32_t>(m_ImageViews.size()); }

		const vk::raii::ImageView& GetImageView(uint32_t index) const { return m_ImageViews[index]; }
		const vk::Image& GetImage(uint32_t index) const { return m_Images[index]; }
		const std::vector<vk::Image>& GetImages() const { return m_Images; }
		const uint32_t GetMinImageCount() const { return m_MinImageCount; }

		vk::raii::Image& GetDepthImage() { return m_DepthImage; }
		vk::raii::ImageView& GetDepthImageView() { return m_DepthImageView; }
		vk::Format& GetDepthFormat() { return m_DepthFormat; }

		void CreateSwapchain(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, uint32_t width, uint32_t height);
		void CreateImageViews(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice);
		void CreateDepthResources(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice);

	private:
		uint32_t FindMemoryType(const vk::raii::PhysicalDevice& physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties);
		vk::Format FindDepthFormat(const vk::raii::PhysicalDevice& physicalDevice);

		std::pair<vk::raii::Image, vk::raii::DeviceMemory> CreateImage(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties);
		vk::raii::ImageView CreateImageView(const vk::raii::Device& device, vk::Image const& image, vk::Format format, vk::ImageAspectFlags aspectFlags);

		vk::raii::SwapchainKHR				 m_Swapchain = nullptr;
		vk::SurfaceFormatKHR				 m_SurfaceFormat;
		vk::Extent2D						 m_Extent;
		std::vector<vk::Image>				 m_Images;
		std::vector<vk::raii::ImageView>	 m_ImageViews;
		uint32_t							 m_MinImageCount;

		vk::raii::Image						 m_DepthImage = nullptr;
		vk::raii::DeviceMemory				 m_DepthImageMemory = nullptr;
		vk::raii::ImageView					 m_DepthImageView = nullptr;
		vk::Format							 m_DepthFormat;

	};

}