#pragma once

#include "RendererAPI.h"

namespace ZEngine {

	class RenderCommand {
	public:
        static void Init(const Ref<UniformBuffer>& cameraUBO) { s_RendererAPI->Init(cameraUBO); }
        static void SetViewport(uint32_t x, uint32_t y, uint32_t w, uint32_t h) { s_RendererAPI->SetViewport(x, y, w, h); }
        static void SetClearColor(const glm::vec4& color) { s_RendererAPI->SetClearColor(color); }

        static void BeginFrame(const Ref<RenderCommandBuffer>& commandBuffer, uint32_t imageIndex) { s_RendererAPI->BeginFrame(commandBuffer, imageIndex); }
        static void EndFrame() { s_RendererAPI->EndFrame(); }
        static void Shutdown() { 
            s_RendererAPI->Shutdown();
            s_RendererAPI.reset();
        }

        static void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t count) {
            s_RendererAPI->DrawIndexed(vertexArray, count);
        }

        static void BindPipelineState(const Ref<PipelineState>& pipelineState) {
            s_RendererAPI->BindPipelineState(pipelineState);
        }

        static void BindGlobalSet(const Ref<PipelineState>& pipelineState) {
            s_RendererAPI->BindGlobalSet(pipelineState);
        }

        static void BindMaterialSet(const Ref<PipelineState>& pipelineState, const Ref<Material>& material) {
            s_RendererAPI->BindMaterialSet(pipelineState, material);
        }

        static void PushConstant(const Ref<PipelineState>& pipelineState, PushConstantData pushConstants) {
            s_RendererAPI->PushConstant(pipelineState, pushConstants);
        }

        static void DrawModel(const Scope<Model>& model, const Ref<PipelineState>& pipelineState) {
            s_RendererAPI->DrawModel(model, pipelineState);
        }

	private:
		static Scope<RendererAPI> s_RendererAPI;
	};
}