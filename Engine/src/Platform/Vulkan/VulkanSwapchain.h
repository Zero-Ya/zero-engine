#pragma once

#include "VulkanContext.h"
#include "ZEngine/Renderer/ResourceManager.h"

namespace ZEngine {
	class ResourceManager;

	class ZE_API VulkanSwapchain {
	public:
		VulkanSwapchain(VulkanContext* ctx, GLFWwindow* window);
		~VulkanSwapchain();

		void init();
		void SetResourceManager(ResourceManager* resource) { resourceManager = resource; };

		vk::raii::SwapchainKHR& getSwapchain() { return swapChain; };
		std::vector<vk::Image> getImages() { return swapChainImages; };
		vk::SurfaceFormatKHR& getFormat() { return swapChainSurfaceFormat; };
		vk::Extent2D getExtent() const { return swapChainExtent; };
		std::vector<vk::raii::ImageView>& getImageViews() { return swapChainImageViews; };

		uint32_t getImageCount() const { return static_cast<uint32_t>(swapChainImageViews.size()); };

		void createSwapChain();
		void createImageViews();
		void cleanupSwapChain();
		void recreateSwapChain();
	private:
		VulkanContext* vk_Ctx = nullptr;
		GLFWwindow* m_Window = nullptr;
		ResourceManager* resourceManager = nullptr;

		vk::raii::SwapchainKHR				 swapChain = nullptr;
		std::vector<vk::Image>				 swapChainImages;
		vk::SurfaceFormatKHR				 swapChainSurfaceFormat;
		vk::Extent2D						 swapChainExtent;
		std::vector<vk::raii::ImageView>	 swapChainImageViews;
		vk::Extent2D chooseSwapExtent(vk::SurfaceCapabilitiesKHR const& capabilities);
	};

}