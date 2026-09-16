#pragma once

#include "RenderCommand.h"
#include "PipelineState.h"
#include "VertexArray.h"
#include "Buffer.h"
#include "Model.h"
#include "Cubemap.h"

#include "PerspectiveCamera.h"
#include "OrthographicCamera.h"

// Camera uniform data
struct CameraData {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

//

namespace ZEngine {

    class Renderer {
    public:
        static void Init();

        static void BeginScene(OrthographicCamera& camera);
        static void BeginScene(PerspectiveCamera& camera);
        static void EndScene();
        static void Shutdown();

        static void DrawModel(const Scope<Model>& model, const Ref<PipelineState>& pipelineState);
        static void DrawMesh(const Ref<VertexArray>& vertexArray);
        static void DrawSkybox(const Scope<Cubemap>& skybox);

        static void GlobalBegin(const Ref<PipelineState>& pipelineState);
        static void MeshBegin(const Ref<PipelineState>& pipelineState, const Ref<Material>& material, const glm::mat4& transform = glm::mat4(1.0f));

        inline static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

        static Ref<UniformBuffer>& GetCameraUBO() { return s_CameraUBO; }
        
    private:
        struct SceneData {
            glm::mat4 ViewProjectionMatrix;
        };

        static Scope<SceneData> s_SceneData;
        static Ref<UniformBuffer> s_CameraUBO;
    };

}