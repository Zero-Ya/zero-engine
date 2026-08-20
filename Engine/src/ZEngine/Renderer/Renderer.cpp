#include "Renderer.h"

namespace ZEngine {

    Scope<Renderer::SceneData> Renderer::s_SceneData = std::make_unique<Renderer::SceneData>();
    Ref<UniformBuffer> Renderer::s_CameraUBO = nullptr;

    void Renderer::Init() {
        s_LayoutManager = LayoutManager::Create();
        s_CameraUBO = UniformBuffer::Create(sizeof(CameraData));
        s_DescriptorAllocator = DescriptorAllocator::Create(s_LayoutManager);

        RenderCommand::Init(s_DescriptorAllocator, s_LayoutManager, s_CameraUBO);
    }

    void Renderer::BeginScene(OrthographicCamera& camera) {
        s_SceneData->ViewProjectionMatrix = camera.GetViewProjectionMatrix();

        CameraData cameraUBO {};
        cameraUBO.model = glm::mat4(1.0f);
        cameraUBO.view = camera.GetViewMatrix();
        cameraUBO.proj = camera.GetProjectionMatrix();
        s_CameraUBO->SetData(&cameraUBO, sizeof(cameraUBO));
    }

    void Renderer::BeginScene(PerspectiveCamera& camera) {
        s_SceneData->ViewProjectionMatrix = camera.GetViewProjectionMatrix();

        CameraData cameraUBO{};
        //cameraUBO.model = camera.GetModelMatrix();
        cameraUBO.model = glm::mat4(1.0f);
        cameraUBO.view = camera.GetViewMatrix();
        cameraUBO.proj = camera.GetProjectionMatrix();
        s_CameraUBO->SetData(&cameraUBO, sizeof(cameraUBO));
    }

    void Renderer::EndScene() {
    }

    void Renderer::Shutdown() {
        s_LayoutManager.reset();
        s_CameraUBO.reset();
        s_SceneData.reset();

        RenderCommand::Shutdown();
        // Descriptor allocator last because rendererAPI still holds descriptor sets
        // Absolutely terrible
        s_DescriptorAllocator.reset();
    }

    void Renderer::Submit(const Ref<PipelineState>& pipelineState,
                          const Ref<VertexArray>& vertexArray,
                          const Ref<Material>& material,
                          const glm::mat4& transform)
    {

        PushConstantData pushConstants {};
        pushConstants.transform = transform;

        RenderCommand::BindPipelineState(pipelineState);
        RenderCommand::BindGlobalSet(pipelineState);
        RenderCommand::BindMaterialSet(pipelineState, material);
        RenderCommand::PushConstant(pipelineState, pushConstants);
        RenderCommand::DrawIndexed(vertexArray, 0);
    }

}