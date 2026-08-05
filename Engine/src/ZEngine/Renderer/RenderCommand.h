#pragma once

#include "RendererAPI.h"

namespace ZEngine {
    class UniformBuffer;
    class PipelineState;

	class RenderCommand {
	public:
        static void Init() { s_RendererAPI->Init(); }
        static void SetViewport(uint32_t x, uint32_t y, uint32_t w, uint32_t h) { s_RendererAPI->SetViewport(x, y, w, h); }
        static void SetClearColor(const glm::vec4& color) { s_RendererAPI->SetClearColor(color); }

        static void BeginFrame(const std::shared_ptr<RenderCommandBuffer>& commandBuffer, uint32_t imageIndex) { s_RendererAPI->BeginFrame(commandBuffer, imageIndex); }
        static void EndFrame() { s_RendererAPI->EndFrame(); }
        static void Shutdown() { 
            s_RendererAPI->Shutdown();
            s_RendererAPI.reset();
        }

        static void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t count) {
            s_RendererAPI->DrawIndexed(vertexArray, count);
        }

        static void BindPipelineState(const std::shared_ptr<PipelineState>& pipelineState) {
            s_RendererAPI->BindPipelineState(pipelineState);
        }
        static void BindDescriptorSets(const std::shared_ptr<PipelineState>& pipelineState, const std::shared_ptr<UniformBuffer>& ubo) {
            s_RendererAPI->BindDescriptorSets(pipelineState, ubo);
        }

	private:
		static std::unique_ptr<RendererAPI> s_RendererAPI;
	};
}