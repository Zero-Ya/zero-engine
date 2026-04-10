#pragma once

#include "VulkanContext.h"

namespace ZEngine {

	class ZE_API VulkanCommandManager {
	public:
		VulkanCommandManager(VulkanContext* ctx);
		~VulkanCommandManager();

		vk::raii::CommandPool& getCommandPool() { return commandPool; };
		std::vector<vk::raii::CommandBuffer>& getCommandBuffers() { return commandBuffers; };

		void createCommandPool();
		void createCommandBuffers();

	private:
		VulkanContext* vk_Ctx;

		vk::raii::CommandPool				 commandPool = nullptr;
		std::vector<vk::raii::CommandBuffer> commandBuffers;

	};

}