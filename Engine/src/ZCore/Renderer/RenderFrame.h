#pragma once

#define VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS

#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanSwapchain.h"
#include "Platform/Vulkan/VulkanSyncManager.h"
#include "Platform/Vulkan/VulkanPipelineManager.h"
#include "Platform/Vulkan/VulkanCommandManager.h"

namespace ZEngine {

	class ZE_API RenderFrame {
	public:
		RenderFrame(VulkanContext* ctx, VulkanSwapchain* swapchain, VulkanSyncManager* sync, VulkanPipelineManager* pipeline, VulkanCommandManager* command);
		~RenderFrame();

		void drawFrame();

		void framebufferResize() { this->framebufferResized = true; };

	private:
		VulkanContext* vk_Ctx;
		VulkanSwapchain* vk_Swapchain;
		VulkanSyncManager* vk_SyncManager;
		VulkanPipelineManager* vk_PipelineManager;
		VulkanCommandManager* vk_CommandManager;

		uint32_t frameIndex = 0;
		bool framebufferResized = false;

		void recordCommandBuffer(uint32_t imageIndex);
		void transition_image_layout(
			uint32_t                imageIndex,
			vk::ImageLayout         old_layout,
			vk::ImageLayout         new_layout,
			vk::AccessFlags2        src_access_mask,
			vk::AccessFlags2        dst_access_mask,
			vk::PipelineStageFlags2 src_stage_mask,
			vk::PipelineStageFlags2 dst_stage_mask);
		void updateUniformBuffer(uint32_t currentImage);

	};

}