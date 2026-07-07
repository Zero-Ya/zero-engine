#pragma once

#define VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS

#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanSwapchain.h"
#include "Platform/Vulkan/VulkanSyncManager.h"
#include "Platform/Vulkan/VulkanPipelineManager.h"
#include "Platform/Vulkan/VulkanCommandManager.h"
#include "ResourceManager.h"

namespace ZEngine {

	class ZE_API RenderFrame {
	public:
		RenderFrame(VulkanContext* ctx,
					VulkanSwapchain* swapchain,
					VulkanSyncManager* sync,
					VulkanPipelineManager* pipeline,
					VulkanCommandManager* command,
					ResourceManager* resource);

		~RenderFrame();

		uint32_t beginFrame();
		void middleRecord(uint32_t imageIndex);
		void endFrame(uint32_t imageIndex);

		void framebufferResize() { this->framebufferResized = true; };
		uint32_t frameIndex = 0;

	private:
		VulkanContext* vk_Ctx;
		VulkanSwapchain* vk_Swapchain;
		VulkanSyncManager* vk_SyncManager;
		VulkanPipelineManager* vk_PipelineManager;
		VulkanCommandManager* vk_CommandManager;
		ResourceManager* resourceManager;

		bool framebufferResized = false;

		void transition_image_layout(
			vk::Image               image,
			vk::ImageLayout         old_layout,
			vk::ImageLayout         new_layout,
			vk::AccessFlags2        src_access_mask,
			vk::AccessFlags2        dst_access_mask,
			vk::PipelineStageFlags2 src_stage_mask,
			vk::PipelineStageFlags2 dst_stage_mask,
			vk::ImageAspectFlags    image_aspect_flags);
		void updateUniformBuffer(uint32_t currentImage);

	};

}