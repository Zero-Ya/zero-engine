#include "VulkanSceneRenderer.h"

#include "Platform/Vulkan/VulkanGraphicsDevice.h"
#include "Platform/Vulkan/VulkanResourceManager.h"
#include "Platform/Vulkan/VulkanBindlessManager.h"

#include "ZEngine/Core/Application.h"
#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanPipelineState.h"

namespace ZEngine {

    Scope<SceneRenderer> SceneRenderer::Create(
        const Scope<GraphicsDevice>& graphicsDevice,
        const Scope<ResourceManager>& resourceManager,
        const Scope<BindlessDescriptorManager>& bindlessManager
    ) {
        return std::make_unique<VulkanSceneRenderer>(graphicsDevice, resourceManager, bindlessManager);
    }

    VulkanSceneRenderer::VulkanSceneRenderer(
        const Scope<GraphicsDevice>& graphicsDevice,
        const Scope<ResourceManager>& resourceManager,
        const Scope<BindlessDescriptorManager>& bindlessManager
    )
    {
        m_GraphicsDevice = static_cast<VulkanGraphicsDevice*>(graphicsDevice.get());
        m_ResourceManager = static_cast<VulkanResourceManager*>(resourceManager.get());
        m_BindlessManager = static_cast<VulkanBindlessManager*>(bindlessManager.get());

        auto& context = m_GraphicsDevice->GetContext();
        auto& device = context.GetDevice();

        const auto maxFramesInFlight = m_GraphicsDevice->GetContext().MAX_FRAMES_IN_FLIGHT;

        // Create scene uniform buffers
        m_SceneUniformBuffers.resize(maxFramesInFlight);
        for (uint32_t i = 0; i < maxFramesInFlight; ++i) {
            BufferCreateInfo createInfo{
                .size = sizeof(CameraData),
                .usageFlags = BufferUsageFlags::UniformBuffer | BufferUsageFlags::TransferDst,
                .isHostVisible = false
            };

            m_SceneUniformBuffers[i] = m_ResourceManager->CreateBuffer(createInfo);
        }

        // Allocate and update scene descriptor sets
        m_SceneDescriptorSets = m_BindlessManager->AllocateSceneSet();

        for (uint32_t i = 0; i < maxFramesInFlight; ++i) {
            auto sceneResource = m_ResourceManager->ResolveBuffer(m_SceneUniformBuffers[i]);

            vk::DescriptorBufferInfo bufferInfo { .buffer = sceneResource->buffer, .offset = 0, .range = sceneResource->size };
            vk::WriteDescriptorSet   descriptorWrite { .dstSet = *m_SceneDescriptorSets[i],
                                                       .dstBinding = 0,
                                                       .dstArrayElement = 0,
                                                       .descriptorCount = 1,
                                                       .descriptorType = vk::DescriptorType::eUniformBuffer,
                                                       .pBufferInfo = &bufferInfo };
            device.updateDescriptorSets(descriptorWrite, nullptr);
        }
    }

    VulkanSceneRenderer::~VulkanSceneRenderer() {
        for (auto& buffer : m_SceneUniformBuffers) {
            if (buffer.IsValid()) {
                m_ResourceManager->DestroyBuffer(buffer);
            }
        }
    }

    void VulkanSceneRenderer::BeginFrame() {
        m_OpaqueDrawQueue.clear();
    }

    void VulkanSceneRenderer::EndFrame() {
    }

    void VulkanSceneRenderer::SubmitModel(const Model& model, const glm::mat4& worldTransform) {
        // Traverse the hierarchical root nodes
        for (const auto& rootNode : model.rootNodes) {
            SubmitNodeRecursive(rootNode, worldTransform, model);
        }
    }

    void VulkanSceneRenderer::SubmitNodeRecursive(const std::shared_ptr<Node>& node, const glm::mat4& parentMatrix, const Model& model) {
        // Compute global transformation matrix by combining parent transform with local node transform
        glm::mat4 globalMatrix = parentMatrix * node->localMatrix;

        // If this node references a mesh, submit its submesh primitives
        if (node->meshIndex != -1 && node->meshIndex < static_cast<int32_t>(model.meshes.size())) {
            const auto& mesh = model.meshes[node->meshIndex];
            for (const auto& submesh : mesh.primitives) {
                SubmitSubmesh(submesh, globalMatrix);
            }
        }

        // Recurse down to all child nodes
        for (const auto& child : node->children) {
            SubmitNodeRecursive(child, globalMatrix, model);
        }
    }

    void VulkanSceneRenderer::SubmitSubmesh(const Submesh& submesh, const glm::mat4& worldTransform) {
        if (submesh.indexCount == 0 || !submesh.vertexBuffer.IsValid()) return;

        DrawCommand cmd;
        cmd.vertexBuffer = submesh.vertexBuffer;
        cmd.indexBuffer = submesh.indexBuffer;
        cmd.indexCount = submesh.indexCount;

        // Populate Push Constants
        cmd.pushConstants.modelMatrix = worldTransform;
        cmd.pushConstants.albedoTextureIndex = submesh.material.albedoTextureIndex;
        cmd.pushConstants.normalTextureIndex = submesh.material.normalTextureIndex;
        cmd.pushConstants.roughnessTextureIndex = 0; // Or material roughness index if mapped
        cmd.pushConstants.samplerIndex = submesh.material.samplerIndex;

        m_OpaqueDrawQueue.push_back(cmd);
    }

    void VulkanSceneRenderer::UpdateSceneBuffers(PerspectiveCamera& camera) {
        auto vk_GraphicsDevice = static_cast<VulkanGraphicsDevice*>(Application::Get().GetGraphicsDevice().get());
        auto& vk_Context = vk_GraphicsDevice->GetContext();
        auto currentFrame = vk_Context.GetCurrentFrameIndex();

        CameraData cameraUBO{};
        cameraUBO.model = glm::mat4(1.0f);
        cameraUBO.view = camera.GetViewMatrix();
        cameraUBO.proj = camera.GetProjectionMatrix();

        m_ResourceManager->UploadBufferData(m_SceneUniformBuffers[currentFrame], &cameraUBO, sizeof(cameraUBO), BufferUsageFlags::UniformBuffer);

        //auto bufferResource = m_ResourceManager->ResolveBuffer(m_SceneUniformBuffers[currentFrame]);
        //uint8_t* destination = static_cast<uint8_t*>(bufferResource->mappedData);
        //std::memcpy(destination, &cameraUBO, sizeof(cameraUBO));
    }

    void VulkanSceneRenderer::RegisterPasses(RenderGraph& renderGraph, PerspectiveCamera& camera, TextureHandle colorTarget, TextureHandle depthTarget) {
        // Update UBO data for the active frame
        UpdateSceneBuffers(camera);

        // Register Opaque Render Pass into the Handle-based RenderGraph using the fluid builder
        renderGraph.AddPass(
            "OpaquePass",
            [&](RenderGraphPassBuilder& builder) {
                builder.WriteColor(colorTarget);
                builder.WriteDepth(depthTarget);
            },
            [this](RHICommandList& cmdList) {
                // Execute drawing commands
                RecordOpaquePass(cmdList);
            }
        );
    }

    void VulkanSceneRenderer::SetPipeline(const Ref<PipelineState>& pipeline) {
        auto& context = m_GraphicsDevice->GetContext();
        const auto& commandBuffer = context.GetCurrentFrame().commandBuffer;

        auto vulkanPipeline = static_cast<VulkanPipelineState*>(pipeline.get());
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, vulkanPipeline->GetNativePipeline());
    }

    void VulkanSceneRenderer::RecordOpaquePass(RHICommandList& cmdList) {
        auto& context = m_GraphicsDevice->GetContext();
        const auto currentFrame = context.GetCurrentFrameIndex();
        const auto& commandBuffer = context.GetCurrentFrame().commandBuffer;

        auto pipelineLayout = m_BindlessManager->GetPipelineLayout();

        // Bind global bindless Set 0 (binding 0: textures, binding 1: samplers...)
        auto bindlessSet = m_BindlessManager->GetDescriptorSet();
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, bindlessSet, nullptr);

        // Bind set 1 (binding 0: uniforms)
        auto& sceneSet = m_SceneDescriptorSets;
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 1, *sceneSet[currentFrame], nullptr);

        // Execute draw calls for all submitted submesh commands
        for (const auto& draw : m_OpaqueDrawQueue) {

            vk::ArrayProxy<const uint8_t> dataProxy(
                sizeof(MeshPushConstants),
                reinterpret_cast<const uint8_t*>(&draw.pushConstants)
            );
            commandBuffer.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, dataProxy);

            auto vertexBuffer = m_ResourceManager->ResolveBuffer(draw.vertexBuffer);
            vk::DeviceSize offset = 0;
            commandBuffer.bindVertexBuffers(0, { vertexBuffer->buffer }, { offset });

            auto indexBuffer = m_ResourceManager->ResolveBuffer(draw.indexBuffer);
            commandBuffer.bindIndexBuffer(indexBuffer->buffer, 0, vk::IndexType::eUint32);

            commandBuffer.drawIndexed(draw.indexCount, 1, 0, 0, 0);
        }
    }

}