#include "Renderer.h"

namespace ZEngine {

    Scope<Renderer::SceneData> Renderer::s_SceneData = std::make_unique<Renderer::SceneData>();
    Ref<UniformBuffer> Renderer::s_CameraUBO = nullptr;
    Ref<UniformBuffer> Renderer::s_TriangleUBO = nullptr;

    void Renderer::Init() {
        RenderCommand::Init();

        s_LayoutManager = ZEngine::LayoutManager::Create();
        s_CameraUBO = UniformBuffer::Create(sizeof(CameraData), SetSlot::Global, 0, s_LayoutManager);
        s_TriangleUBO = UniformBuffer::Create(sizeof(ObjectData), SetSlot::Object, 0, s_LayoutManager);
    }

    void Renderer::BeginScene(PerspectiveCamera& camera) {
        s_SceneData->ViewProjectionMatrix = camera.GetViewProjectionMatrix();

        CameraData cameraUBO{};
        cameraUBO.model = camera.GetModelMatrix();
        cameraUBO.view = camera.GetViewMatrix();
        cameraUBO.proj = camera.GetProjectionMatrix();
        s_CameraUBO->SetData(&cameraUBO, sizeof(cameraUBO));
    }

    void Renderer::EndScene() {
    }

    void Renderer::Shutdown() {
        s_LayoutManager.reset();
        s_TriangleUBO.reset();
        s_CameraUBO.reset();
        s_SceneData.reset();

        RenderCommand::Shutdown();
    }

    void Renderer::Submit(const Ref<PipelineState>& pipelineState,
                          const Ref<VertexArray>& vertexArray,
                          const glm::mat4& transform)
    {

        ObjectData objectUBO{};
        objectUBO.model = transform;
        objectUBO.normal = glm::mat4(1.0f);
        s_TriangleUBO->SetData(&objectUBO, sizeof(objectUBO));

        RenderCommand::BindPipelineState(pipelineState);
        RenderCommand::BindDescriptorSets(pipelineState, s_CameraUBO);
        RenderCommand::BindDescriptorSets(pipelineState, s_TriangleUBO);
        RenderCommand::DrawIndexed(vertexArray, 0);
    }

}