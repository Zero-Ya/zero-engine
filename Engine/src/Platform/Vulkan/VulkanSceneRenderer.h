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

        // Frame Lifecycle
        void BeginFrame() override;
        void EndFrame() override;

        // Hierarchical Model & Submesh Submission
        void SubmitModel(const Model& model, const glm::mat4& worldTransform) override;
        void SubmitSubmesh(const Submesh& submesh, const glm::mat4& worldTransform) override;

        // RenderGraph Pass Registration
        void RegisterPasses(RenderGraph& renderGraph, PerspectiveCamera& camera, TextureHandle colorTarget, TextureHandle depthTarget) override;

        void SetPipeline(const Ref<PipelineState>& pipeline) override;

        void RecordOpaquePass(RHICommandList& cmdList) override;

    private:
        void SubmitNodeRecursive(const std::shared_ptr<Node>& node, const glm::mat4& parentMatrix, const Model& model);
        void UpdateSceneBuffers(PerspectiveCamera& camera) override;

        VulkanGraphicsDevice* m_GraphicsDevice = nullptr;
        VulkanResourceManager* m_ResourceManager = nullptr;
        VulkanBindlessManager* m_BindlessManager = nullptr;

        std::vector<BufferHandle> m_SceneUniformBuffers;
        std::vector<vk::raii::DescriptorSet> m_SceneDescriptorSets;

        std::vector<DrawCommand> m_OpaqueDrawQueue;

    };

}