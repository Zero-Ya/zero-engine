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
    class ShadowMap;
    class Cubemap;

    // Internal draw command extracted from tree traversal
    struct DrawCommand {
        BufferHandle vertexBuffer;
        BufferHandle indexBuffer;
        uint32_t indexCount = 0;

        MeshPushConstants pushConstants;
        AABB localAABB;
    };

    class SceneRenderer {
    public:
        ~SceneRenderer() = default;

        // Frame lifecycle
        virtual void BeginFrame() = 0;
        virtual void EndFrame() = 0;

        // Hierarchical model & submesh submission
        virtual void SubmitModel(const Model& model, const glm::mat4& worldTransform) = 0;
        virtual void SubmitSubmesh(const Submesh& submesh, const glm::mat4& worldTransform) = 0;

        virtual void UpdateSceneBuffers(const DirectionalLight& lighting, PerspectiveCamera& camera, const glm::mat4& lightSpaceMatrix) = 0;
        virtual glm::mat4 ComputeLightSpaceMatrix(const DirectionalLight& lighting, PerspectiveCamera& camera) = 0;

        virtual void UpdateDrawQueue(PerspectiveCamera& camera) = 0;
        virtual void UpdateShadowDrawQueue(const glm::mat4& lightViewProj) = 0;

        // Render graph pass registration
        virtual void RegisterPasses(RenderGraph& renderGraph, PerspectiveCamera& camera, TextureHandle colorTarget, TextureHandle depthTarget) = 0;

        virtual void SetPipeline(const Ref<PipelineState>& pipeline) = 0;

        virtual void RecordShadowPass(RHICommandList& cmdList, const Ref<ShadowMap>& shadowMap, const glm::mat4& lightSpaceMatrix) = 0;
        virtual void RecordOpaquePass(RHICommandList& cmdList, const Ref<ShadowMap>& shadowMap) = 0;
        virtual void RecordSkyboxPass(RHICommandList& cmdList, const Scope<Cubemap>& cubemap) = 0;

        static Scope<SceneRenderer>Create(const Scope<GraphicsDevice>& graphicsDevice,
                                          const Scope<ResourceManager>& resourceManager,
                                          const Scope<BindlessDescriptorManager>& bindlessManager);
    };

}