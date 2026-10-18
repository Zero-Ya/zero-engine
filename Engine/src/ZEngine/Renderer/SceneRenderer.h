#pragma once

#include "ZEngine/Asset/ShaderTypes.h"
#include "ZEngine/Asset/Model.h"

#include "PerspectiveCamera.h"

#include "RenderGraph.h"

namespace ZEngine {

    class GraphicsDevice;
    class ResourceManager;
    class BindlessDescriptorManager;

    class RHICommandList;
    class PipelineState;

    // Internal draw command extracted from tree traversal
    struct DrawCommand {
        BufferHandle vertexBuffer;
        BufferHandle indexBuffer;
        uint32_t indexCount = 0;

        MeshPushConstants pushConstants;
    };

    class SceneRenderer {
    public:
        ~SceneRenderer() = default;

        // Frame Lifecycle
        virtual void BeginFrame() = 0;
        virtual void EndFrame() = 0;

        // Hierarchical Model & Submesh Submission
        virtual void SubmitModel(const Model& model, const glm::mat4& worldTransform) = 0;
        virtual void SubmitSubmesh(const Submesh& submesh, const glm::mat4& worldTransform) = 0;

        virtual void UpdateSceneBuffers(PerspectiveCamera& camera) = 0;

        // RenderGraph Pass Registration
        virtual void RegisterPasses(RenderGraph& renderGraph, PerspectiveCamera& camera, TextureHandle colorTarget, TextureHandle depthTarget) = 0;

        virtual void SetPipeline(const Ref<PipelineState>& pipeline) = 0;

        virtual void RecordOpaquePass(RHICommandList& cmdList) = 0;

        static Scope<SceneRenderer>Create(const Scope<GraphicsDevice>& graphicsDevice,
                                          const Scope<ResourceManager>& resourceManager,
                                          const Scope<BindlessDescriptorManager>& bindlessManager);
    };

}