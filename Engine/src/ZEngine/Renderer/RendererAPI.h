#pragma once

#include "RenderCommandBuffer.h"
#include "ZEngine/Renderer/VertexArray.h"

#include <glm/glm.hpp>

namespace ZEngine {
	class PipelineState;
	class UniformBuffer;

	class RendererAPI {
	public:
		enum class API { None = 0, Vulkan = 1 };

	public:
		virtual ~RendererAPI() = default;

		virtual void Init() = 0;
		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
		virtual void SetClearColor(const glm::vec4& color) = 0;
		virtual void Clear() = 0;

		virtual void BeginFrame(const Ref<RenderCommandBuffer>& commandBuffer, uint32_t imageIndex) = 0;
		virtual void EndFrame() = 0;
		virtual void Shutdown() = 0;

		virtual void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0) = 0;

		virtual void BindPipelineState(const Ref<PipelineState>& pipelineState) = 0;
		virtual void BindDescriptorSets(const Ref<PipelineState>& pipelineState, const Ref<UniformBuffer>& ubo) = 0;

		static API GetAPI() { return s_API; }
		static Scope<RendererAPI> Create();

	private:
		static API s_API;
	};

}