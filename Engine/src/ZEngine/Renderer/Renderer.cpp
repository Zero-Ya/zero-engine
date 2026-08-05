#include "Renderer.h"

namespace ZEngine {

    std::unique_ptr<Renderer::SceneData> Renderer::s_SceneData = std::make_unique<Renderer::SceneData>();
    std::shared_ptr<UniformBuffer> Renderer::s_CameraUBO = nullptr;

    void Renderer::Init(const std::unique_ptr<LayoutManager>& layoutManager) {
        RenderCommand::Init();

        s_CameraUBO = UniformBuffer::Create(sizeof(UniformBufferObject), 0, 0, layoutManager);
    }

    void Renderer::BeginScene(PerspectiveCamera& camera) {
        s_SceneData->ViewProjectionMatrix = camera.GetViewProjectionMatrix();

        UniformBufferObject UBO{};
        UBO.model = camera.GetModelMatrix();
        UBO.view = camera.GetViewMatrix();
        UBO.proj = camera.GetProjectionMatrix();

        s_CameraUBO->SetData(&UBO, sizeof(UBO), 0);
    }

    void Renderer::EndScene() {
    }

    void Renderer::Shutdown() {
        s_CameraUBO.reset();
        s_SceneData.reset();

        RenderCommand::Shutdown();
    }

    void Renderer::Submit(const std::shared_ptr<PipelineState>& pipelineState,
                          const std::shared_ptr<VertexArray>& vertexArray)
    {
        RenderCommand::BindPipelineState(pipelineState);
        RenderCommand::BindDescriptorSets(pipelineState, s_CameraUBO);
        RenderCommand::DrawIndexed(vertexArray, 0);
    }

}