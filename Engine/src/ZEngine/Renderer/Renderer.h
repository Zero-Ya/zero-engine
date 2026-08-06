#pragma once

#include "RenderCommand.h"
#include "PipelineState.h"
#include "VertexArray.h"
#include "Buffer.h"

#include "LayoutManager.h"
#include "PerspectiveCamera.h"

namespace ZEngine {

    class Renderer {
    public:
        static void Init();

        static void BeginScene(PerspectiveCamera& camera);
        static void EndScene();
        static void Shutdown();

        static void Submit(const std::shared_ptr<PipelineState>& pipelineState,
                           const std::shared_ptr<VertexArray>& vertexArray);

        inline static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

        static std::unique_ptr<LayoutManager>& GetLayoutManager() { return s_LayoutManager; }
        
    private:
        struct SceneData {
            glm::mat4 ViewProjectionMatrix;
        };

        static inline std::unique_ptr<LayoutManager> s_LayoutManager = nullptr;
        static std::unique_ptr<SceneData> s_SceneData;
        static std::shared_ptr<UniformBuffer> s_CameraUBO;
    };

}