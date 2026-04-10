#include "VulkanSyncManager.h"

namespace ZEngine {

	VulkanSyncManager::VulkanSyncManager(VulkanContext* ctx, VulkanSwapchain* vkSwapchain) : vk_Ctx(ctx), vk_Swapchain(vkSwapchain) {}

	VulkanSyncManager::~VulkanSyncManager() {}

	void VulkanSyncManager::init()
	{
		createSyncObjects();
	}

	void VulkanSyncManager::createSyncObjects()
	{
		assert(presentCompleteSemaphores.empty() && renderFinishedSemaphores.empty() && inFlightFences.empty());

		for (size_t i = 0; i < vk_Swapchain->getImageCount(); i++)
		{
			renderFinishedSemaphores.emplace_back(vk_Ctx->getDevice(), vk::SemaphoreCreateInfo());
		}

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			presentCompleteSemaphores.emplace_back(vk_Ctx->getDevice(), vk::SemaphoreCreateInfo());
			inFlightFences.emplace_back(vk_Ctx->getDevice(), vk::FenceCreateInfo{ .flags = vk::FenceCreateFlagBits::eSignaled });
		}
	}

}