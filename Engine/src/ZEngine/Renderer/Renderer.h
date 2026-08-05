#pragma once

#include "RenderCommand.h"
#include "PipelineState.h"
#include "VertexArray.h"
#include "Buffer.h"

#include "PerspectiveCamera.h"

namespace ZEngine {

    class LayoutManager;

    class Renderer {
    public:
        static void Init(const std::unique_ptr<LayoutManager>& layoutManager);

        static void BeginScene(PerspectiveCamera& camera);
        static void EndScene();
        static void Shutdown();

        static void Submit(const std::shared_ptr<PipelineState>& pipelineState,
                           const std::shared_ptr<VertexArray>& vertexArray);

        inline static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

    private:
        struct SceneData {
            glm::mat4 ViewProjectionMatrix;
        };

        static std::unique_ptr<SceneData> s_SceneData;
        static std::shared_ptr<UniformBuffer> s_CameraUBO;
    };

}