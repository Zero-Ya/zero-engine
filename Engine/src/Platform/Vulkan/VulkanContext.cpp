#include "VulkanContext.h"
#include "Platform/Vulkan/VulkanSwapchain.h"
#include "Platform/Vulkan/VulkanCommandBuffer.h"

namespace {

	uint32_t FindMemoryType(vk::raii::PhysicalDevice physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties);
	std::pair<vk::raii::Image, vk::raii::DeviceMemory> CreateImage(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties);
	vk::raii::ImageView CreateImageView(const vk::raii::Device& device, vk::Image const& image, vk::Format format, vk::ImageAspectFlags aspectFlags);

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
		CreateSwapchain();
		CreateCommandPool();
		CreateDepthResources();
		CreateSyncObjects();
		ZE_CORE_INFO("Vulkan Context created!");
	}

	void VulkanContext::SwapBuffers() {
	}

	void VulkanContext::CreateInstance() {
		constexpr vk::ApplicationInfo appInfo { .pApplicationName = "Zero Engine",
											    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
											    .pEngineName = "No Engine",
											    .engineVersion = VK_MAKE_VERSION(1, 0, 0),
											    .apiVersion = vk::ApiVersion14 };

		// Get the required layers
		std::vector<char const*> requiredLayers;
		if (enableValidationLayers)
		{
			requiredLayers.assign(validationLayers.begin(), validationLayers.end());
		}

		// Check if the required layers are supported by the Vulkan implementation.
		auto layerProperties = m_Context.enumerateInstanceLayerProperties();
		auto unsupportedLayerIt = std::ranges::find_if(requiredLayers,
			[&layerProperties](auto const& requiredLayer) {
				return std::ranges::none_of(layerProperties,
					[requiredLayer](auto const& layerProperty) { return strcmp(layerProperty.layerName, requiredLayer) == 0; });
			});
		if (unsupportedLayerIt != requiredLayers.end())
		{
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
		if (unsupportedPropertyIt != requiredExtensions.end())
		{
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
		if (glfwCreateWindowSurface(*m_Instance, m_Window, nullptr, &_surface) != 0)
		{
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
			features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

		// Return true if the physicalDevice meets all the criteria
		return supportsVulkan1_3 && supportsGraphics && supportsAllRequiredExtensions && supportsRequiredFeatures;
	}

	void VulkanContext::PickPhysicalDevice() {
		std::vector<vk::raii::PhysicalDevice> physicalDevices = m_Instance.enumeratePhysicalDevices();
		auto const                            devIter = std::ranges::find_if(physicalDevices, [&](auto const& physicalDevice) { return IsDeviceSuitable(physicalDevice); });
		if (devIter == physicalDevices.end())
		{
			throw std::runtime_error("failed to find a suitable GPU!");
		}
		m_PhysicalDevice = *devIter;
	}

	void VulkanContext::CreateLogicalDevice() {
		std::vector<vk::QueueFamilyProperties> queueFamilyProperties = m_PhysicalDevice.getQueueFamilyProperties();

		// Get the first index into queueFamilyProperties which supports both graphics and present
		for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); qfpIndex++)
		{
			if ((queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) &&
				m_PhysicalDevice.getSurfaceSupportKHR(qfpIndex, *m_Surface))
			{
				// Found a queue family that supports both graphics and present
				queueIndex = qfpIndex;
				break;
			}
		}
		if (queueIndex == ~0)
		{
			throw std::runtime_error("Could not find a queue for graphics and present -> terminating");
		}

		// Query for Vulkan 1.3 features
		vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain = {
			{.features = {.samplerAnisotropy = true}},
			{.synchronization2 = true, .dynamicRendering = true},
			{.extendedDynamicState = true}
		};

		// Create a Device
		float                     queuePriority = 0.5f;
		vk::DeviceQueueCreateInfo deviceQueueCreateInfo{ .queueFamilyIndex = queueIndex, .queueCount = 1, .pQueuePriorities = &queuePriority };
		vk::DeviceCreateInfo      deviceCreateInfo{ .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
												   .queueCreateInfoCount = 1,
												   .pQueueCreateInfos = &deviceQueueCreateInfo,
												   .enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtension.size()),
												   .ppEnabledExtensionNames = requiredDeviceExtension.data() };

		m_Device = vk::raii::Device(m_PhysicalDevice, deviceCreateInfo);
		m_GraphicsQueue = vk::raii::Queue(m_Device, queueIndex, 0);
	}

	void VulkanContext::CreateSwapchain() {
		int width, height;
		glfwGetWindowSize(m_Window, &width, &height);
		m_Swapchain = std::make_unique<VulkanSwapchain>(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
	}

	void VulkanContext::CreateCommandPool() {
		vk::CommandPoolCreateInfo poolInfo { .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
			   .queueFamilyIndex = queueIndex };
		m_CommandPool = vk::raii::CommandPool(m_Device, poolInfo);
	}

	void VulkanContext::CreateDepthResources() {
		m_DepthFormat = FindDepthFormat();

		std::tie(m_DepthImage, m_DepthImageMemory) = CreateImage(m_Device, m_PhysicalDevice, m_Swapchain->GetExtent().width, m_Swapchain->GetExtent().height, m_DepthFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal);
		m_DepthImageView = CreateImageView(m_Device, m_DepthImage, m_DepthFormat, vk::ImageAspectFlagBits::eDepth);
	}

	void VulkanContext::CreateSyncObjects() {
		vk::SemaphoreCreateInfo semaphoreInfo;
		vk::FenceCreateInfo fenceInfo { .flags = vk::FenceCreateFlagBits::eSignaled }; // Start signaled so first frame doesn't freeze

		for (int i = 0; i < m_Swapchain->GetImages().size(); i++) {
			m_RenderFinishedSemaphores.emplace_back(m_Device, semaphoreInfo);
		}

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			m_ImageAvailableSemaphores.emplace_back(m_Device, semaphoreInfo);
			m_InFlightFences.emplace_back(m_Device, fenceInfo);
		}
	}

	vk::Format VulkanContext::FindSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) {
		for (const auto format : candidates) {
			vk::FormatProperties props = m_PhysicalDevice.getFormatProperties(format);
			if (((tiling == vk::ImageTiling::eLinear) && ((props.linearTilingFeatures & features) == features)) ||
				((tiling == vk::ImageTiling::eOptimal) && ((props.optimalTilingFeatures & features) == features))) {
				return format;
			}
		}

		throw std::runtime_error("failed to find supported format!");
	}

	vk::Format VulkanContext::FindDepthFormat() {
		return FindSupportedFormat({ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
			vk::ImageTiling::eOptimal,
			vk::FormatFeatureFlagBits::eDepthStencilAttachment);
	}

	uint32_t VulkanContext::AcquireNextImage() {
		// 1. Wait until the GPU has completely finished processing this specific frame slot from last loop
		auto fenceResult = m_Device.waitForFences(*m_InFlightFences[m_CurrentFrameIndex], vk::True, UINT64_MAX);
		if (fenceResult != vk::Result::eSuccess) {
			ZE_CORE_ERROR("Failed to wait for fence!");
		}

		// 2. Request the next index from the swapchain
		// When the image becomes ready, the GPU will signal our m_ImageAvailableSemaphore
		auto [result, imageIndex] = m_Swapchain->GetSwapchain().acquireNextImage(
			UINT64_MAX, *m_ImageAvailableSemaphores[m_CurrentFrameIndex], nullptr );

		if (result == vk::Result::eErrorOutOfDateKHR) {
			RecreateSwapchain();
			return imageIndex;
		}
		if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
			assert(result == vk::Result::eTimeout || result == vk::Result::eNotReady);
			ZE_CORE_ERROR("Failed to acquire swap chain image!");
		}

		// Update uniform buffer when we implement them later

		// 3. Open our active command buffer for recording fresh draw commands
		m_Device.resetFences(*m_InFlightFences[m_CurrentFrameIndex]);

		return imageIndex;
	}

	void VulkanContext::PresentImage(uint32_t imageIndex, const Ref<RenderCommandBuffer>& renderCommandBuffer) {
		auto vulkanCommandBuffer = static_cast<VulkanCommandBuffer*>(renderCommandBuffer.get());
		const auto& commandBuffer = vulkanCommandBuffer->GetBuffer();

		// Submit the recorded drawing packet to the GPU graphics hardware queue
		vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

		vk::SubmitInfo submitInfo {
			.waitSemaphoreCount   = 1,
			.pWaitSemaphores	  = &(*m_ImageAvailableSemaphores[m_CurrentFrameIndex]), // Wait until image is acquired
			.pWaitDstStageMask	  = &waitStages[0],
			.commandBufferCount	  = 1,
			.pCommandBuffers	  = &(*commandBuffer), // Run these draw commands
			.signalSemaphoreCount = 1,
			.pSignalSemaphores	  = &(*m_RenderFinishedSemaphores[imageIndex]) }; // Signal when done drawing

		m_GraphicsQueue.submit(submitInfo, *m_InFlightFences[m_CurrentFrameIndex]);

		// Hand the completed image back to the monitor engine presentation queue
		vk::PresentInfoKHR presentInfo {
			.waitSemaphoreCount = 1,
			.pWaitSemaphores	= &(*m_RenderFinishedSemaphores[imageIndex]), // Wait until GPU finishes rendering
			.swapchainCount		= 1,
			.pSwapchains		= &(*m_Swapchain->GetSwapchain()), // Target swapchain
			.pImageIndices		= &imageIndex };

		auto presentResult = m_GraphicsQueue.presentKHR(presentInfo);
		if ((presentResult == vk::Result::eSuboptimalKHR) || (presentResult == vk::Result::eErrorOutOfDateKHR)) {
			// Framebuffer resize stuff here
			RecreateSwapchain();
		}

		// Advance our tracking cycle index to the next synchronization pool slot
		m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void VulkanContext::RecreateSwapchain() {
		int width, height;
		glfwGetWindowSize(m_Window, &width, &height);

		if (width == 0 || height == 0) return;
		m_Device.waitIdle();

		m_Swapchain->CleanupSwapchain();
		m_Swapchain->CreateSwapchain(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
		m_Swapchain->CreateImageViews();
		CreateDepthResources();
	}

	void VulkanContext::WaitIdle() {
		m_Device.waitIdle();
	}

}

namespace {

	uint32_t FindMemoryType(vk::raii::PhysicalDevice physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
		vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}

		throw std::runtime_error("failed to find suitable memory type!");
	}

	std::pair<vk::raii::Image, vk::raii::DeviceMemory> CreateImage(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties) {
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

	vk::raii::ImageView CreateImageView(const vk::raii::Device& device, vk::Image const& image, vk::Format format, vk::ImageAspectFlags aspectFlags) {
		vk::ImageViewCreateInfo viewInfo{
			.image = image,
			.viewType = vk::ImageViewType::e2D,
			.format = format,
			.subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1} };
		return vk::raii::ImageView(device, viewInfo);
	}

	VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void*) {
		if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError || severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
		{
			std::cerr << "validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;
		}

		return vk::False;
	}

	std::vector<const char*> getRequiredInstanceExtensions(bool enableValidationLayers) {
		uint32_t glfwExtensionCount = 0;
		auto     glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
		if (enableValidationLayers)
		{
			extensions.push_back(vk::EXTDebugUtilsExtensionName);
		}

		return extensions;
	}

}