#pragma once

#include "ZEngine/Renderer/SceneRenderer.h"

#include <vulkan/vulkan_raii.hpp>

namespace ZEngine {

    class VulkanGraphicsDevice;
    class VulkanResourceManager;
    class VulkanBindlessManager;

    class VulkanSceneRenderer : public SceneRenderer {
    public:
        VulkanSceneRenderer(
            const Scope<GraphicsDevice>& graphicsDevice,
            const Scope<ResourceManager>& resourceManager,
            const Scope<BindlessDescriptorManager>& bindlessManager
        );
        ~VulkanSceneRenderer();

        // Frame lifecycle
        void BeginFrame() override;
        void EndFrame() override;

        // Hierarchical model and submesh submission
        void SubmitModel(const Model& model, const glm::mat4& worldTransform) override;
        void SubmitSubmesh(const Submesh& submesh, const glm::mat4& worldTransform) override;

        void UpdateDrawQueue(PerspectiveCamera& camera) override;
        void UpdateShadowDrawQueue(const glm::mat4& lightViewProj) override;

        // Render graph pass registration
        void RegisterPasses(RenderGraph& renderGraph, PerspectiveCamera& camera, TextureHandle colorTarget, TextureHandle depthTarget) override;

        void SetPipeline(const Ref<PipelineState>& pipeline) override;

        void RecordShadowPass(RHICommandList& cmdList, const Ref<ShadowMap>& shadowMap, const glm::mat4& lightSpaceMatrix) override;
        void RecordOpaquePass(RHICommandList& cmdList, const Ref<ShadowMap>& shadowMap) override;
        void RecordSkyboxPass(RHICommandList& cmdList, const Scope<Cubemap>& cubemap) override;

    private:
        void SubmitNodeRecursive(const std::shared_ptr<Node>& node, const glm::mat4& parentMatrix, const Model& model);

        void UpdateSceneBuffers(const DirectionalLight& lighting, PerspectiveCamera& camera, const glm::mat4& lightSpaceMatrix) override;
        glm::mat4 ComputeLightSpaceMatrix(const DirectionalLight& lighting, PerspectiveCamera& camera) override;

        VulkanGraphicsDevice* m_GraphicsDevice = nullptr;
        VulkanResourceManager* m_ResourceManager = nullptr;
        VulkanBindlessManager* m_BindlessManager = nullptr;

        std::vector<BufferHandle> m_SceneUniformBuffers;
        std::vector<vk::raii::DescriptorSet> m_SceneDescriptorSets;

        std::vector<DrawCommand> m_OpaqueDrawQueue;

        std::vector<DrawCommand> m_VisibleDrawQueue;
        std::vector<DrawCommand> m_ShadowVisibleDrawQueue;
    };

}