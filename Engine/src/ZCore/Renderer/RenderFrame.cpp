#include "RenderFrame.h"

namespace ZEngine {

	RenderFrame::RenderFrame(VulkanContext* ctx, VulkanSwapchain* swapchain, VulkanSyncManager* sync, VulkanPipelineManager* pipeline, VulkanCommandManager* command)
		: vk_Ctx(ctx), vk_Swapchain(swapchain), vk_SyncManager(sync), vk_PipelineManager(pipeline), vk_CommandManager(command)
	{
	};

	RenderFrame::~RenderFrame() {};

	void RenderFrame::drawFrame()
	{
		// Note: inFlightFences, presentCompleteSemaphores, and commandBuffers are indexed by frameIndex,
		//       while renderFinishedSemaphores is indexed by imageIndex
		auto fenceResult = vk_Ctx->getDevice().waitForFences(*vk_SyncManager->getInFlightFences()[frameIndex], vk::True, UINT64_MAX);
		if (fenceResult != vk::Result::eSuccess)
		{
			throw std::runtime_error("failed to wait for fence!");
		}

		auto [result, imageIndex] = vk_Swapchain->getSwapchain().acquireNextImage(UINT64_MAX, *vk_SyncManager->getPresentSemaphores()[frameIndex], nullptr);

		// Due to VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS being defined, eErrorOutOfDateKHR can be checked as a result
		// here and does not need to be caught by an exception.
		if (result == vk::Result::eErrorOutOfDateKHR)
		{
			vk_Swapchain->recreateSwapChain();
			return;
		}
		// On other success codes than eSuccess and eSuboptimalKHR we just throw an exception.
		// On any error code, aquireNextImage already threw an exception.
		if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
		{
			assert(result == vk::Result::eTimeout || result == vk::Result::eNotReady);
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		//updateUniformBuffer(frameIndex); // Wait until uniform buffer implementation

		// Only reset the fence if we are submitting work
		vk_Ctx->getDevice().resetFences(*vk_SyncManager->getInFlightFences()[frameIndex]);

		vk_CommandManager->getCommandBuffers()[frameIndex].reset();
		recordCommandBuffer(imageIndex);

		vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
		const vk::SubmitInfo   submitInfo{ .waitSemaphoreCount = 1,
										  .pWaitSemaphores = &*vk_SyncManager->getPresentSemaphores()[frameIndex],
										  .pWaitDstStageMask = &waitDestinationStageMask,
										  .commandBufferCount = 1,
										  .pCommandBuffers = &*vk_CommandManager->getCommandBuffers()[frameIndex],
										  .signalSemaphoreCount = 1,
										  .pSignalSemaphores = &*vk_SyncManager->getRenderSemaphores()[imageIndex] };
		vk_Ctx->getQueue().submit(submitInfo, *vk_SyncManager->getInFlightFences()[frameIndex]);

		const vk::PresentInfoKHR presentInfoKHR{ .waitSemaphoreCount = 1,
												.pWaitSemaphores = &*vk_SyncManager->getRenderSemaphores()[imageIndex],
												.swapchainCount = 1,
												.pSwapchains = &*vk_Swapchain->getSwapchain(),
												.pImageIndices = &imageIndex };
		result = vk_Ctx->getQueue().presentKHR(presentInfoKHR);
		// Due to VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS being defined, eErrorOutOfDateKHR can be checked as a result
		// here and does not need to be caught by an exception.
		if ((result == vk::Result::eSuboptimalKHR) || (result == vk::Result::eErrorOutOfDateKHR) || framebufferResized)
		{
			framebufferResized = false;
			vk_Swapchain->recreateSwapChain();
		}
		else
		{
			// There are no other success codes than eSuccess; on any error code, presentKHR already threw an exception.
			assert(result == vk::Result::eSuccess);
		}
		frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
	}
	
	void RenderFrame::recordCommandBuffer(uint32_t imageIndex)
	{
		auto& commandBuffer = vk_CommandManager->getCommandBuffers()[frameIndex];
		commandBuffer.begin({});

		// Before starting rendering, transition the swapchain image to vk::ImageLayout::eColorAttachmentOptimal
		transition_image_layout(
			imageIndex,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eColorAttachmentOptimal,
			{},                                                        // srcAccessMask (no need to wait for previous operations)
			vk::AccessFlagBits2::eColorAttachmentWrite,                // dstAccessMask
			vk::PipelineStageFlagBits2::eColorAttachmentOutput,        // srcStage
			vk::PipelineStageFlagBits2::eColorAttachmentOutput         // dstStage
		);
		vk::ClearValue              clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
		vk::RenderingAttachmentInfo attachmentInfo = {
			.imageView = vk_Swapchain->getImageViews()[imageIndex],
			.imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
			.loadOp = vk::AttachmentLoadOp::eClear,
			.storeOp = vk::AttachmentStoreOp::eStore,
			.clearValue = clearColor };
		vk::RenderingInfo renderingInfo = {
			.renderArea = {.offset = {0, 0}, .extent = vk_Swapchain->getExtent()},
			.layerCount = 1,
			.colorAttachmentCount = 1,
			.pColorAttachments = &attachmentInfo };

		commandBuffer.beginRendering(renderingInfo);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *vk_PipelineManager->getGraphicsPipeline());
		commandBuffer.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(vk_Swapchain->getExtent().width), static_cast<float>(vk_Swapchain->getExtent().height), 0.0f, 1.0f));
		commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), vk_Swapchain->getExtent()));
		//commandBuffer.bindVertexBuffers(0, *vertexBuffer, { 0 });
		//commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, *descriptorSets[frameIndex], nullptr);
		//commandBuffer.bindIndexBuffer(*indexBuffer, 0, vk::IndexTypeValue<decltype(indices)::value_type>::value);
		commandBuffer.draw(3, 1, 0, 0);
		commandBuffer.endRendering();

		// After rendering, transition the swapchain image to vk::ImageLayout::ePresentSrcKHR
		transition_image_layout(
			imageIndex,
			vk::ImageLayout::eColorAttachmentOptimal,
			vk::ImageLayout::ePresentSrcKHR,
			vk::AccessFlagBits2::eColorAttachmentWrite,                // srcAccessMask
			{},                                                        // dstAccessMask
			vk::PipelineStageFlagBits2::eColorAttachmentOutput,        // srcStage
			vk::PipelineStageFlagBits2::eBottomOfPipe                  // dstStage
		);
		commandBuffer.end();
	}

	void RenderFrame::transition_image_layout(
		uint32_t                imageIndex,
		vk::ImageLayout         old_layout,
		vk::ImageLayout         new_layout,
		vk::AccessFlags2        src_access_mask,
		vk::AccessFlags2        dst_access_mask,
		vk::PipelineStageFlags2 src_stage_mask,
		vk::PipelineStageFlags2 dst_stage_mask)
	{
		vk::ImageMemoryBarrier2 barrier = {
			.srcStageMask = src_stage_mask,
			.srcAccessMask = src_access_mask,
			.dstStageMask = dst_stage_mask,
			.dstAccessMask = dst_access_mask,
			.oldLayout = old_layout,
			.newLayout = new_layout,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = vk_Swapchain->getImages()[imageIndex],
			.subresourceRange = {
				   .aspectMask = vk::ImageAspectFlagBits::eColor,
				   .baseMipLevel = 0,
				   .levelCount = 1,
				   .baseArrayLayer = 0,
				   .layerCount = 1} };
		vk::DependencyInfo dependency_info = {
			.dependencyFlags = {},
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = &barrier };
		vk_CommandManager->getCommandBuffers()[frameIndex].pipelineBarrier2(dependency_info);
	}

	//void RenderFrame::updateUniformBuffer(uint32_t currentImage)
	//{
	//	static auto startTime = std::chrono::high_resolution_clock::now();

	//	auto  currentTime = std::chrono::high_resolution_clock::now();
	//	float time = std::chrono::duration<float>(currentTime - startTime).count();

	//	UniformBufferObject ubo{};
	//	ubo.model = rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	//	ubo.view = lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	//	ubo.proj = glm::perspective(glm::radians(45.0f), static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height), 0.1f, 10.0f);
	//	ubo.proj[1][1] *= -1;

	//	memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
	//}

}