#pragma once

#include "ZEngine/Renderer/RendererAPI.h"

#include <vulkan/vulkan_raii.hpp>

namespace ZEngine {
	class LayoutManager;
	class UniformBuffer;
	class PipelineState;

	class VulkanRendererAPI : public RendererAPI {
	public:
		virtual void Init() override;
		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
		virtual void SetClearColor(const glm::vec4& color) override;
		virtual void Clear() override;

		virtual void BeginFrame(const std::shared_ptr<RenderCommandBuffer>& commandBuffer, uint32_t imageIndex) override;
		virtual void EndFrame() override;
		virtual void Shutdown() override;

		virtual void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount = 0) override;

		virtual void BindPipelineState(const std::shared_ptr<PipelineState>& pipelineState) override;
		virtual void BindDescriptorSets(const std::shared_ptr<PipelineState>& pipelineState, const std::shared_ptr<UniformBuffer>& ubo) override;

	private:
		void TransitionImageLayout(
			vk::Image               image,
			vk::ImageLayout         old_layout,
			vk::ImageLayout         new_layout,
			vk::AccessFlags2        src_access_mask,
			vk::AccessFlags2        dst_access_mask,
			vk::PipelineStageFlags2 src_stage_mask,
			vk::PipelineStageFlags2 dst_stage_mask,
			vk::ImageAspectFlags    image_aspect_flags );

	private:
		glm::vec4 m_ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };

		std::shared_ptr<RenderCommandBuffer> m_ActiveCommandBuffer = nullptr;
		uint32_t m_CurrentImageIndex = 0;
	};

}