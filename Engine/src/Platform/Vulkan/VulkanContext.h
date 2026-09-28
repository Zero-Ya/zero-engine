#pragma once

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

	class LayoutManager;
	class DescriptorAllocator;

	struct FrameData {
		vk::raii::CommandPool commandPool = nullptr;
		vk::raii::CommandBuffer commandBuffer = nullptr;

		vk::raii::Semaphore imageAvailableSemaphore = nullptr;
		vk::raii::Fence inFlightFence = nullptr;
	};

	class VulkanContext {
	public:
		VulkanContext(GLFWwindow* window);
		~VulkanContext();

		void Init();

		// Getters
		vk::raii::Instance&		  GetInstance()			 { return m_Instance; }
		vk::raii::SurfaceKHR&	  GetSurface()			 { return m_Surface; }
		vk::raii::PhysicalDevice& GetPhysicalDevice()	 { return m_PhysicalDevice; }
		vk::raii::Device&		  GetDevice()			 { return m_Device; }
		uint32_t				  GetQueueIndex() const  { return m_QueueIndex; }
		vk::raii::Queue			  GetGraphicsQueue()	 { return m_GraphicsQueue; }

		Scope<LayoutManager>& GetLayoutManager() { return m_LayoutManager; }
		Scope<DescriptorAllocator>& GetDescriptorAllocator() { return m_DescriptorAllocator; }

		void BeginFrame();
		void EndFrame(uint32_t imageIndex);
		vk::Result Present(uint32_t imageIndex, const vk::raii::SwapchainKHR& swapchain);

		void WaitIdle() { m_Device.waitIdle(); }
		void QueueWaitIdle() { m_GraphicsQueue.waitIdle(); }

		FrameData& GetCurrentFrame() { return m_Frames[m_CurrentFrameIndex]; }
		const uint32_t GetCurrentFrameIndex() const { return m_CurrentFrameIndex; }

		static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

		void CreateFrameResources();

	private:
		void CreateInstance();
		void SetupDebugMessenger();
		void CreateSurface();
		bool IsDeviceSuitable(vk::raii::PhysicalDevice const& physicalDevice);
		void PickPhysicalDevice();
		void CreateLogicalDevice();

	private:
		GLFWwindow*						 m_Window			= nullptr;

		vk::raii::Context				 m_Context;
		vk::raii::Instance				 m_Instance			= nullptr;
		vk::raii::DebugUtilsMessengerEXT debugMessenger		= nullptr;
		vk::raii::SurfaceKHR			 m_Surface			= nullptr;
		vk::raii::PhysicalDevice		 m_PhysicalDevice	= nullptr;
		vk::raii::Device				 m_Device			= nullptr;

		uint32_t						 m_QueueIndex		= 0;
		vk::raii::Queue					 m_GraphicsQueue	= nullptr;

		Scope<LayoutManager> m_LayoutManager;
		Scope<DescriptorAllocator> m_DescriptorAllocator;

		std::array<FrameData, MAX_FRAMES_IN_FLIGHT> m_Frames;
		uint32_t m_CurrentFrameIndex = 0;

		std::vector<vk::raii::Semaphore> m_RenderFinishedSemaphores;

		const std::vector<char const*> validationLayers = {
			"VK_LAYER_KHRONOS_validation" };

		std::vector<const char*> requiredDeviceExtension = {
			vk::KHRSwapchainExtensionName,
			vk::KHRDynamicRenderingExtensionName
		};
	};

}