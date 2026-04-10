#include "VulkanCommandManager.h"

namespace ZEngine {

	VulkanCommandManager::VulkanCommandManager(VulkanContext* ctx) : vk_Ctx(ctx)
	{
	}

	VulkanCommandManager::~VulkanCommandManager() {}

	void VulkanCommandManager::createCommandPool()
	{
		vk::CommandPoolCreateInfo poolInfo{ .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
					   .queueFamilyIndex = vk_Ctx->getQueueIndex()};
		commandPool = vk::raii::CommandPool(vk_Ctx->getDevice(), poolInfo);
	}

	void VulkanCommandManager::createCommandBuffers()
	{
		vk::CommandBufferAllocateInfo allocInfo{ .commandPool = commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = MAX_FRAMES_IN_FLIGHT };
		commandBuffers = vk::raii::CommandBuffers(vk_Ctx->getDevice(), allocInfo);
	}

}