#pragma once

#include "VulkanContext.h"
#include "VulkanSwapchain.h"

namespace ZEngine {

	class ZE_API VulkanSyncManager {
	public:
		VulkanSyncManager(VulkanContext* ctx, VulkanSwapchain* vkSwapchain);
		~VulkanSyncManager();

		// Explicitly tell the compiler the class is move-only and safe to move
		VulkanSyncManager(VulkanSyncManager&&) noexcept = default;
		VulkanSyncManager& operator=(VulkanSyncManager&&) noexcept = default;

		// Delete copy operations
		VulkanSyncManager(const VulkanSyncManager&) = delete;
		VulkanSyncManager& operator=(const VulkanSyncManager&) = delete;

		void init();

		std::vector<vk::raii::Semaphore>& getPresentSemaphores() { return presentCompleteSemaphores; };
		std::vector<vk::raii::Semaphore>& getRenderSemaphores() { return renderFinishedSemaphores; };
		std::vector<vk::raii::Fence>& getInFlightFences() { return inFlightFences; };

	private:
		VulkanContext* vk_Ctx;
		VulkanSwapchain* vk_Swapchain;

		std::vector<vk::raii::Semaphore>	 presentCompleteSemaphores;
		std::vector<vk::raii::Semaphore>	 renderFinishedSemaphores;
		std::vector<vk::raii::Fence>		 inFlightFences;

		void createSyncObjects();
	};

}