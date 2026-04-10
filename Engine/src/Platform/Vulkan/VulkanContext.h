#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace ZEngine {
	constexpr int MAX_FRAMES_IN_FLIGHT = 2;

	const std::vector<char const*> validationLayers = {
		"VK_LAYER_KHRONOS_validation" };

#ifdef ZE_DEBUG
	constexpr bool enableValidationLayers = true;
#else
	constexpr bool enableValidationLayers = false;
#endif

	class ZE_API VulkanContext {
	public:
		VulkanContext(GLFWwindow* window);
		~VulkanContext();

		VulkanContext(const VulkanContext&) = delete;
		VulkanContext& operator=(const VulkanContext&) = delete;

		void init();

		vk::raii::Instance& getInstance() { return instance; };
		vk::raii::SurfaceKHR& getSurface() { return surface; };

		vk::raii::PhysicalDevice getPhysicalDevice() { return physicalDevice; };
		vk::raii::Device& getDevice() { return device; };

		uint32_t getQueueIndex() { return queueIndex; };
		vk::raii::Queue getQueue() { return queue; };

	private:
		GLFWwindow* m_Window = nullptr;

		vk::raii::Context  context;
		vk::raii::Instance instance						 = nullptr;
		vk::raii::DebugUtilsMessengerEXT debugMessenger  = nullptr;
		vk::raii::SurfaceKHR surface					 = nullptr;
		vk::raii::PhysicalDevice physicalDevice			 = nullptr;
		vk::raii::Device device							 = nullptr;
		uint32_t queueIndex								 = ~0;
		vk::raii::Queue queue							 = nullptr;

		std::vector<const char*> requiredDeviceExtension = {
			vk::KHRSwapchainExtensionName };

		void createInstance();
		void setupDebugMessenger();
		void createSurface();
		bool isDeviceSuitable(vk::raii::PhysicalDevice const& physicalDevice);
		void pickPhysicalDevice();
		void createLogicalDevice();
	};

}