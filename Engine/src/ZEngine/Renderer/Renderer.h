#pragma once

#include "RenderCommand.h"
#include "PipelineState.h"
#include "VertexArray.h"
#include "Buffer.h"

#include "LayoutManager.h"
#include "PerspectiveCamera.h"

// Temporary uniform data
struct CameraData {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

struct ObjectData {
    glm::mat4 model;
    glm::mat4 normal;
};

//

namespace ZEngine {

    class Renderer {
    public:
        static void Init();

        static void BeginScene(PerspectiveCamera& camera);
        static void EndScene();
        static void Shutdown();

        static void Submit(const Ref<PipelineState>& pipelineState,
                           const Ref<VertexArray>& vertexArray,
                           const glm::mat4& transform = glm::mat4(1.0f));

        inline static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

        static Scope<LayoutManager>& GetLayoutManager() { return s_LayoutManager; }
        
    private:
        struct SceneData {
            glm::mat4 ViewProjectionMatrix;
        };

        static inline Scope<LayoutManager> s_LayoutManager = nullptr;
        static Scope<SceneData> s_SceneData;
        static Ref<UniformBuffer> s_CameraUBO;
        static Ref<UniformBuffer> s_TriangleUBO;
    };

}