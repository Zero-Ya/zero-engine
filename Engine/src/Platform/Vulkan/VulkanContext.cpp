#include "VulkanContext.h"

#include "ZEngine/Core/Application.h"
#include "VulkanGraphicsDevice.h"
#include "VulkanSwapchain.h"

#include "ZEngine/Renderer/LayoutManager.h"
#include "ZEngine/Renderer/DescriptorAllocator.h"

namespace {

	std::vector<const char*> getRequiredInstanceExtensions(bool enableValidationLayers);
	VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void*);

}

namespace ZEngine {

	VulkanContext::VulkanContext(GLFWwindow* window)
		: m_Window(window) {}

	VulkanContext::~VulkanContext() {}

	void VulkanContext::Init() {
		CreateInstance();
		SetupDebugMessenger();
		CreateSurface();
		PickPhysicalDevice();
		CreateLogicalDevice();
		//CreateFrameResources();

		m_LayoutManager = LayoutManager::Create();
		m_DescriptorAllocator = DescriptorAllocator::Create();

		ZE_CORE_INFO("Vulkan Context created!");
	}

	void VulkanContext::CreateInstance() {
		constexpr vk::ApplicationInfo appInfo { .pApplicationName = "Zero Engine",
											    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
											    .pEngineName = "No Engine",
											    .engineVersion = VK_MAKE_VERSION(1, 0, 0),
											    .apiVersion = vk::ApiVersion14 };

		// Get the required layers
		std::vector<char const*> requiredLayers;
		if (enableValidationLayers) {
			requiredLayers.assign(validationLayers.begin(), validationLayers.end());
		}

		// Check if the required layers are supported by the Vulkan implementation.
		auto layerProperties = m_Context.enumerateInstanceLayerProperties();
		auto unsupportedLayerIt = std::ranges::find_if(requiredLayers,
			[&layerProperties](auto const& requiredLayer) {
				return std::ranges::none_of(layerProperties,
					[requiredLayer](auto const& layerProperty) { return strcmp(layerProperty.layerName, requiredLayer) == 0; });
			});
		if (unsupportedLayerIt != requiredLayers.end()) {
			throw std::runtime_error("Required layer not supported: " + std::string(*unsupportedLayerIt));
		}

		// Get the required extensions.
		auto requiredExtensions = getRequiredInstanceExtensions(enableValidationLayers);

		// Check if the required extensions are supported by the Vulkan implementation.
		auto extensionProperties = m_Context.enumerateInstanceExtensionProperties();
		auto unsupportedPropertyIt =
			std::ranges::find_if(requiredExtensions,
				[&extensionProperties](auto const& requiredExtension) {
					return std::ranges::none_of(extensionProperties,
						[requiredExtension](auto const& extensionProperty) { return strcmp(extensionProperty.extensionName, requiredExtension) == 0; });
				});
		if (unsupportedPropertyIt != requiredExtensions.end()) {
			throw std::runtime_error("Required extension not supported: " + std::string(*unsupportedPropertyIt));
		}

		vk::InstanceCreateInfo createInfo { .pApplicationInfo = &appInfo,
										    .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
										    .ppEnabledLayerNames = requiredLayers.data(),
										    .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
										    .ppEnabledExtensionNames = requiredExtensions.data() };

		m_Instance = vk::raii::Instance(m_Context, createInfo);
	}

	void VulkanContext::SetupDebugMessenger() {
		if (!enableValidationLayers)
			return;

		vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
		vk::DebugUtilsMessageTypeFlagsEXT     messageTypeFlags(
			vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
		vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{ .messageSeverity = severityFlags,
																			  .messageType = messageTypeFlags,
																			  .pfnUserCallback = &debugCallback };
		debugMessenger = m_Instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
	}

	void VulkanContext::CreateSurface() {
		VkSurfaceKHR _surface;
		if (glfwCreateWindowSurface(*m_Instance, m_Window, nullptr, &_surface) != 0) {
			throw std::runtime_error("failed to create window surface!");
		}
		m_Surface = vk::raii::SurfaceKHR(m_Instance, _surface);
	}

	bool VulkanContext::IsDeviceSuitable(vk::raii::PhysicalDevice const& physicalDevice) {
		// Check if the physicalDevice supports the Vulkan 1.3 API version
		bool supportsVulkan1_3 = physicalDevice.getProperties().apiVersion >= VK_API_VERSION_1_3;

		// Check if any of the queue families support graphics operations
		auto queueFamilies = physicalDevice.getQueueFamilyProperties();
		bool supportsGraphics = std::ranges::any_of(queueFamilies, [](auto const& qfp) { return !!(qfp.queueFlags & vk::QueueFlagBits::eGraphics); });

		// Check if all required physicalDevice extensions are available
		auto availableDeviceExtensions = physicalDevice.enumerateDeviceExtensionProperties();
		bool supportsAllRequiredExtensions =
			std::ranges::all_of(requiredDeviceExtension,
				[&availableDeviceExtensions](auto const& requiredDeviceExtension) {
					return std::ranges::any_of(availableDeviceExtensions,
						[requiredDeviceExtension](auto const& availableDeviceExtension) { return strcmp(availableDeviceExtension.extensionName, requiredDeviceExtension) == 0; });
				});

		// Check if the physicalDevice supports the required features
		auto features = physicalDevice.template getFeatures2<vk::PhysicalDeviceFeatures2,
			vk::PhysicalDeviceVulkan11Features,
			vk::PhysicalDeviceVulkan13Features,
			vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
		bool supportsRequiredFeatures = features.template get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy &&
			features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
			features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState &&
			features.template get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters;

		// Return true if the physicalDevice meets all the criteria
		return supportsVulkan1_3 && supportsGraphics && supportsAllRequiredExtensions && supportsRequiredFeatures;
	}

	void VulkanContext::PickPhysicalDevice() {
		std::vector<vk::raii::PhysicalDevice> physicalDevices = m_Instance.enumeratePhysicalDevices();
		auto const                            devIter = std::ranges::find_if(physicalDevices, [&](auto const& physicalDevice) { return IsDeviceSuitable(physicalDevice); });
		if (devIter == physicalDevices.end()) {
			throw std::runtime_error("failed to find a suitable GPU!");
		}
		m_PhysicalDevice = *devIter;
	}

	void VulkanContext::CreateLogicalDevice() {
		std::vector<vk::QueueFamilyProperties> queueFamilyProperties = m_PhysicalDevice.getQueueFamilyProperties();

		// Get the first index into queueFamilyProperties which supports both graphics and present
		for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); qfpIndex++) {
			if ((queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) &&
				m_PhysicalDevice.getSurfaceSupportKHR(qfpIndex, *m_Surface)) {
				// Found a queue family that supports both graphics and present
				m_QueueIndex = qfpIndex;
				break;
			}
		}
		if (m_QueueIndex == ~0) {
			throw std::runtime_error("Could not find a queue for graphics and present -> terminating");
		}

		// Query for Vulkan 1.3 features
		vk::StructureChain<vk::PhysicalDeviceFeatures2,
						   vk::PhysicalDeviceVulkan13Features,
						   vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT,
						   vk::PhysicalDeviceVulkan11Features>
		featureChain = {
			{.features = {.samplerAnisotropy = true}},
			{.synchronization2 = true, .dynamicRendering = true},
			{.extendedDynamicState = true},
			{.shaderDrawParameters = true}
		};

		// Create a Device
		float                     queuePriority = 0.5f;
		vk::DeviceQueueCreateInfo deviceQueueCreateInfo { .queueFamilyIndex = m_QueueIndex, .queueCount = 1, .pQueuePriorities = &queuePriority };
		vk::DeviceCreateInfo      deviceCreateInfo { .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
												     .queueCreateInfoCount = 1,
												     .pQueueCreateInfos = &deviceQueueCreateInfo,
												     .enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtension.size()),
												     .ppEnabledExtensionNames = requiredDeviceExtension.data() };

		m_Device = vk::raii::Device(m_PhysicalDevice, deviceCreateInfo);
		m_GraphicsQueue = vk::raii::Queue(m_Device, m_QueueIndex, 0);
	}

	void VulkanContext::CreateFrameResources() {
		auto vk_GraphicsDevice = static_cast<VulkanGraphicsDevice*>(Application::Get().GetGraphicsDevice().get());
		auto& vk_Swapchain = vk_GraphicsDevice->GetSwapchain();

		vk::SemaphoreCreateInfo semaphoreInfo{};
		vk::FenceCreateInfo fenceInfo{ .flags = vk::FenceCreateFlagBits::eSignaled };

		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			// Create command pool
			vk::CommandPoolCreateInfo poolInfo {
				.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
				.queueFamilyIndex = m_QueueIndex
			};
			m_Frames[i].commandPool = vk::raii::CommandPool(m_Device, poolInfo);

			// Create command buffer
			vk::CommandBufferAllocateInfo allocInfo {
				.commandPool = *m_Frames[i].commandPool,
				.level = vk::CommandBufferLevel::ePrimary,
				.commandBufferCount = 1
			};
			m_Frames[i].commandBuffer = std::move(vk::raii::CommandBuffers(m_Device, allocInfo)[0]);

			// Create sync objects
			m_Frames[i].imageAvailableSemaphore = vk::raii::Semaphore(m_Device, semaphoreInfo);
			m_Frames[i].inFlightFence = vk::raii::Fence(m_Device, fenceInfo);
		}

		// Standalone because of swapchain image size
		for (uint32_t i = 0; i < vk_Swapchain.GetImageCount(); ++i) {
			m_RenderFinishedSemaphores.emplace_back(m_Device, semaphoreInfo);
		}
	}

	void VulkanContext::BeginFrame() {
		auto& frame = GetCurrentFrame();

		auto fenceResult = m_Device.waitForFences(*frame.inFlightFence, vk::True, UINT64_MAX);
		if (fenceResult != vk::Result::eSuccess) {
			ZE_CORE_ERROR("Failed to wait for fence!");
		}
		m_Device.resetFences(*frame.inFlightFence);

		frame.commandBuffer.reset();
		vk::CommandBufferBeginInfo beginInfo{ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
		frame.commandBuffer.begin(beginInfo);
	}

	void VulkanContext::EndFrame(uint32_t imageIndex) {
		auto& frame = GetCurrentFrame();
		frame.commandBuffer.end();

		// Submit the recorded drawing packet to the GPU graphics hardware queue
		vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

		vk::SubmitInfo submitInfo {
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &(*frame.imageAvailableSemaphore), // Wait until image is acquired
			.pWaitDstStageMask = &waitStages[0],
			.commandBufferCount = 1,
			.pCommandBuffers = &(*frame.commandBuffer), // Run these draw commands
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &(*m_RenderFinishedSemaphores[imageIndex])}; // Signal when done drawing

		m_GraphicsQueue.submit(submitInfo, frame.inFlightFence);
	}


	vk::Result VulkanContext::Present(uint32_t imageIndex, const vk::raii::SwapchainKHR& swapchain) {
		// Hand the completed image back to the monitor engine presentation queue
		vk::PresentInfoKHR presentInfo {
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &(*m_RenderFinishedSemaphores[imageIndex]), // Wait until GPU finishes rendering
			.swapchainCount = 1,
			.pSwapchains = &(*swapchain), // Target swapchain
			.pImageIndices = &imageIndex };

		auto presentResult = m_GraphicsQueue.presentKHR(presentInfo);
		if ((presentResult == vk::Result::eSuboptimalKHR) || (presentResult == vk::Result::eErrorOutOfDateKHR)) {
			return presentResult;
		}

		// Advance our tracking cycle index to the next synchronization pool slot
		m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
		return presentResult;
	}

}

namespace {

	VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void*) {
		if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError || severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
			std::cerr << "validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;
		}

		return vk::False;
	}

	std::vector<const char*> getRequiredInstanceExtensions(bool enableValidationLayers) {
		uint32_t glfwExtensionCount = 0;
		auto     glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
		if (enableValidationLayers) {
			extensions.push_back(vk::EXTDebugUtilsExtensionName);
		}

		return extensions;
	}

}