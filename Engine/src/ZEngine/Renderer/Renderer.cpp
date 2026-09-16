#include "Renderer.h"

#include "Renderer2D.h"

namespace ZEngine {

    Scope<Renderer::SceneData> Renderer::s_SceneData = std::make_unique<Renderer::SceneData>();
    Ref<UniformBuffer> Renderer::s_CameraUBO = nullptr;

    void Renderer::Init() {
        s_CameraUBO = UniformBuffer::Create(sizeof(CameraData));

        RenderCommand::Init(s_CameraUBO);
        Renderer2D::Init();
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
        s_CameraUBO.reset();
        s_SceneData.reset();

        Renderer2D::Shutdown();
        RenderCommand::Shutdown();
    }

    void Renderer::DrawModel(const Scope<Model>& model, const Ref<PipelineState>& pipelineState) {
        RenderCommand::DrawModel(model, pipelineState);
    }

    void Renderer::DrawMesh(const Ref<VertexArray>& vertexArray) {
        RenderCommand::DrawIndexed(vertexArray, 0);
    }

    void Renderer::DrawSkybox(const Scope<Cubemap>& skybox) {
        RenderCommand::DrawSkybox(skybox);
    }

    void Renderer::GlobalBegin(const Ref<PipelineState>& pipelineState) {
        RenderCommand::BindPipelineState(pipelineState);
        RenderCommand::BindGlobalSet(pipelineState);
    }
    
    void Renderer::MeshBegin(const Ref<PipelineState>& pipelineState, const Ref<Material>& material, const glm::mat4& transform) {
        PushConstantData pushConstants{};
        pushConstants.transform = transform;

        if (material != nullptr)
            RenderCommand::BindMaterialSet(pipelineState, material);
        RenderCommand::PushConstant(pipelineState, pushConstants);
    }

}