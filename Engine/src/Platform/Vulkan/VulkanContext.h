#pragma once

#include "ZEngine/Renderer/GraphicsContext.h"

#define VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS
#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace ZEngine {
	// Validation layer
	#ifdef ZE_DEBUG
		constexpr bool enableValidationLayers = true;
	#else
		constexpr bool enableValidationLayers = false;
	#endif

	// Forward declaration
	class RenderCommandBuffer;
	class VulkanSwapchain;

	// Actual class
	class VulkanContext : public GraphicsContext {
	public:
		VulkanContext(GLFWwindow* window);
		~VulkanContext();

		virtual void Init() override;
		virtual void SwapBuffers() override;

		// Getters
		vk::raii::Instance&		 GetInstance()		 { return m_Instance; }
		vk::raii::SurfaceKHR&	 GetSurface()		 { return m_Surface; }
		vk::raii::PhysicalDevice& GetPhysicalDevice() { return m_PhysicalDevice; }
		// Get logical device
		vk::raii::Device&		 GetDevice()		 { return m_Device; }
		uint32_t				 GetQueueIndex()	 { return queueIndex; }
		vk::raii::Queue			 GetGraphicsQueue()  { return m_GraphicsQueue; }
		vk::raii::CommandPool&	 GetCommandPool()	 { return m_CommandPool; }

		//std::unique_ptr<VulkanSwapchain>& GetSwapchain() { return m_Swapchain; }
		VulkanSwapchain* GetSwapchain() { return m_Swapchain.get(); }

		uint32_t AcquireNextImage() override;
		void PresentImage(uint32_t imageIndex, const std::shared_ptr<RenderCommandBuffer>& renderCommandBuffer) override;
		void RecreateSwapchain() override;
		void WaitIdle() override;

		void QueueWaitIdle() { m_GraphicsQueue.waitIdle(); }
		const uint32_t GetCurrentFrameIndex() const { return m_CurrentFrameIndex; }
		const uint32_t GetMaxFramesInFlight() const { return MAX_FRAMES_IN_FLIGHT; }

	private:
		void CreateInstance();
		void SetupDebugMessenger();
		void CreateSurface();
		bool IsDeviceSuitable(vk::raii::PhysicalDevice const& physicalDevice);
		void PickPhysicalDevice();
		void CreateLogicalDevice();
		void CreateSwapchain();
		void CreateCommandPool();
		void CreateSyncObjects();

	private:
		GLFWwindow*						 m_Window			= nullptr;

		vk::raii::Context				 m_Context;
		vk::raii::Instance				 m_Instance			= nullptr;
		vk::raii::DebugUtilsMessengerEXT debugMessenger		= nullptr;
		vk::raii::SurfaceKHR			 m_Surface			= nullptr;
		vk::raii::PhysicalDevice		 m_PhysicalDevice	= nullptr;
		vk::raii::Device				 m_Device			= nullptr;

		uint32_t						 queueIndex			= ~0;
		vk::raii::Queue					 m_GraphicsQueue	= nullptr;
		vk::raii::CommandPool			 m_CommandPool		= nullptr;

		std::unique_ptr<VulkanSwapchain> m_Swapchain;

		std::vector<vk::raii::Semaphore> m_ImageAvailableSemaphores;
		std::vector<vk::raii::Semaphore> m_RenderFinishedSemaphores;
		std::vector<vk::raii::Fence> m_InFlightFences;

		uint32_t m_CurrentFrameIndex = 0;
		const uint32_t MAX_FRAMES_IN_FLIGHT = 2; // Double buffering synchronization tracking

		const std::vector<char const*> validationLayers = {
			"VK_LAYER_KHRONOS_validation" };

		std::vector<const char*> requiredDeviceExtension = {
			vk::KHRSwapchainExtensionName,
			vk::KHRDynamicRenderingExtensionName
		};
	};

}