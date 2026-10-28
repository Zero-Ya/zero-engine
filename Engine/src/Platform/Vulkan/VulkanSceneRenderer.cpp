#include "VulkanSceneRenderer.h"

#include "VulkanGraphicsDevice.h"
#include "VulkanResourceManager.h"
#include "VulkanBindlessManager.h"

#include "VulkanContext.h"
#include "VulkanPipelineState.h"

#include "VulkanShadowMap.h"
#include "VulkanCubemap.h"

#include "ZEngine/Core/Frustum.h"

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
                .size = sizeof(SceneUniformData),
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
        cmd.localAABB = submesh.localAABB;

        // Populate Push Constants
        cmd.pushConstants.modelMatrix = worldTransform;
        cmd.pushConstants.albedoTextureIndex = submesh.material.albedoTextureIndex;
        cmd.pushConstants.normalTextureIndex = submesh.material.normalTextureIndex;
        cmd.pushConstants.roughnessTextureIndex = 0; // Or material roughness index if mapped
        cmd.pushConstants.samplerIndex = submesh.material.samplerIndex;

        m_OpaqueDrawQueue.push_back(cmd);
    }

    void VulkanSceneRenderer::UpdateDrawQueue(PerspectiveCamera& camera) {
        m_VisibleDrawQueue.clear();

        // Extract camera frustum planes
        glm::mat4 viewProj = camera.GetProjectionMatrix() * camera.GetViewMatrix();
        Frustum camFrustum = camera.ExtractFrustum(viewProj);

        // Frustum cull
        for (const auto& pending : m_OpaqueDrawQueue) {
            AABB worldAABB = pending.localAABB.Transform(pending.pushConstants.modelMatrix);

            if (camFrustum.IsOnFrustum(worldAABB)) {
                DrawCommand finalCmd;
                finalCmd.vertexBuffer = pending.vertexBuffer;
                finalCmd.indexBuffer = pending.indexBuffer;
                finalCmd.indexCount = pending.indexCount;
                finalCmd.pushConstants = pending.pushConstants;

                m_VisibleDrawQueue.push_back(finalCmd);
            }
        }

        m_OpaqueDrawQueue.clear();
    }

    void VulkanSceneRenderer::UpdateShadowDrawQueue(const glm::mat4& lightViewProj) {
        m_ShadowVisibleDrawQueue.clear();

        // Extract frustum planes
        Frustum lightFrustum = Frustum::ExtractFrustumFromMatrix(lightViewProj);

        // Frustum culll
        for (const auto& pending : m_OpaqueDrawQueue) {
            AABB worldAABB = pending.localAABB.Transform(pending.pushConstants.modelMatrix);

            if (lightFrustum.IsOnFrustum(worldAABB)) {
                DrawCommand shadowCmd;
                shadowCmd.vertexBuffer = pending.vertexBuffer;
                shadowCmd.indexBuffer = pending.indexBuffer;
                shadowCmd.indexCount = pending.indexCount;
                shadowCmd.pushConstants = pending.pushConstants;

                m_ShadowVisibleDrawQueue.push_back(shadowCmd);
            }
        }
    }

    void VulkanSceneRenderer::UpdateSceneBuffers(const DirectionalLight& lighting, PerspectiveCamera& camera, const glm::mat4& lightSpaceMatrix) {
        auto& context = m_GraphicsDevice->GetContext();
        auto currentFrame = context.GetCurrentFrameIndex();

        SceneUniformData ubo{};
        ubo.camera.model = glm::mat4(1.0f);
        ubo.camera.view = camera.GetViewMatrix();
        ubo.camera.proj = camera.GetProjectionMatrix();

        ubo.sunLight.direction = normalize(lighting.direction);
        ubo.sunLight.intensity = lighting.intensity;
        ubo.sunLight.color = lighting.color;

        ubo.cameraPos = glm::vec4(camera.GetPosition(), 1.0f);

        ubo.lightSpaceMatrix = lightSpaceMatrix;

        m_ResourceManager->UploadBufferData(m_SceneUniformBuffers[currentFrame], &ubo, sizeof(ubo), BufferUsageFlags::UniformBuffer);
    }

    void VulkanSceneRenderer::RegisterPasses(RenderGraph& renderGraph, PerspectiveCamera& camera, TextureHandle colorTarget, TextureHandle depthTarget) {
        // Update UBO data for the active frame
        //UpdateSceneBuffers(camera);

        // Register Opaque Render Pass into the Handle-based RenderGraph using the fluid builder
        renderGraph.AddPass(
            "OpaquePass",
            [&](RenderGraphPassBuilder& builder) {
                builder.WriteColor(colorTarget);
                builder.WriteDepth(depthTarget);
            },
            [this](RHICommandList& cmdList) {
                // Execute drawing commands
                //RecordOpaquePass(cmdList);
            }
        );
    }

    void VulkanSceneRenderer::SetPipeline(const Ref<PipelineState>& pipeline) {
        auto& context = m_GraphicsDevice->GetContext();
        const auto& commandBuffer = context.GetCurrentFrame().commandBuffer;

        auto vulkanPipeline = static_cast<VulkanPipelineState*>(pipeline.get());
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, vulkanPipeline->GetNativePipeline());
    }

    void VulkanSceneRenderer::RecordOpaquePass(RHICommandList& cmdList, const Ref<ShadowMap>& shadowMap) {
        auto& context = m_GraphicsDevice->GetContext();
        const auto currentFrame = context.GetCurrentFrameIndex();
        const auto& commandBuffer = context.GetCurrentFrame().commandBuffer;

        auto vk_ShadowMap = static_cast<VulkanShadowMap*>(shadowMap.get());
        auto pipelineLayout = m_BindlessManager->GetPipelineLayout();

        // Bind global bindless Set 0 (binding 0: textures, binding 1: samplers...)
        auto bindlessSet = m_BindlessManager->GetDescriptorSet();
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, bindlessSet, nullptr);

        // Bind set 1 (binding 0: uniforms)
        auto& sceneSet = m_SceneDescriptorSets;
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 1, *sceneSet[currentFrame], nullptr);

        // Execute draw calls for all submitted submesh commands
        for (const auto& draw : m_OpaqueDrawQueue) { // Supposed to be m_VisibleDrawQueue

            ShadowMapData shadowMapData {
                .textureIndex = vk_ShadowMap->GetTextureIndex(),
                .samplerIndex = vk_ShadowMap->GetSamplerIndex()
            };

            PushConstantData pushConstants {
                .meshConstants = draw.pushConstants,
                .shadowMapData = shadowMapData
            };

            vk::ArrayProxy<const uint8_t> dataProxy(
                sizeof(PushConstantData),
                reinterpret_cast<const uint8_t*>(&pushConstants)
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

    glm::mat4 VulkanSceneRenderer::ComputeLightSpaceMatrix(const DirectionalLight& lighting, PerspectiveCamera& camera) {
        // Anchor the light target to the camera's position (or focus point)
        glm::vec3 lightTarget = camera.GetPosition();
        
        // Position the light source back from the camera along the light direction
        float lightDistance = 50.0f;
        glm::vec3 lightPosition = lightTarget - (glm::normalize(lighting.direction) * lightDistance);
        
        // Create light view matrix
        glm::vec3 up = (std::abs(glm::normalize(lighting.direction).y) > 0.99f) ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
        glm::mat4 lightView = glm::lookAt(lightPosition, lightTarget, up);
        
        // Orthographic bounds
        float orthoSize = 100.0f;
        float nearPlane = -50.0f; // Negative so that objects behind light targets aren't clipped
        float farPlane = 150.0f;
        
        glm::mat4 lightProj = glm::orthoZO(-orthoSize, orthoSize, -orthoSize, orthoSize, nearPlane, farPlane);
        lightProj[1][1] *= -1.0f; // Vulkan Y-inversion
        
        return lightProj * lightView;
    }

    void VulkanSceneRenderer::RecordShadowPass(RHICommandList& cmdList, const Ref<ShadowMap>& shadowMap, const glm::mat4& lightSpaceMatrix) {
        auto& context = m_GraphicsDevice->GetContext();
        const auto currentFrame = context.GetCurrentFrameIndex();
        const auto& commandBuffer = context.GetCurrentFrame().commandBuffer;

        auto vk_ShadowMap = static_cast<VulkanShadowMap*>(shadowMap.get());
        auto pipelineLayout = vk_ShadowMap->GetPipelineLayout();

        // Bind shadow depth-only pipeline
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, vk_ShadowMap->GetPipeline());

        for (const auto& draw : m_OpaqueDrawQueue) { // Supposed to be m_VisibleDrawQueue
            ShadowPushConstants pushConstants {};
            pushConstants.lightSpaceMatrix = lightSpaceMatrix;
            pushConstants.modelMatrix = draw.pushConstants.modelMatrix;

            vk::ArrayProxy<const uint8_t> dataProxy(
                sizeof(ShadowPushConstants),
                reinterpret_cast<const uint8_t*>(&pushConstants)
            );
            commandBuffer.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, dataProxy);

            auto vertexBuffer = m_ResourceManager->ResolveBuffer(draw.vertexBuffer);
            vk::DeviceSize offset = 0;
            commandBuffer.bindVertexBuffers(0, { vertexBuffer->buffer }, { offset });

            auto indexBuffer = m_ResourceManager->ResolveBuffer(draw.indexBuffer);
            commandBuffer.bindIndexBuffer(indexBuffer->buffer, 0, vk::IndexType::eUint32);

            commandBuffer.drawIndexed(draw.indexCount, 1, 0, 0, 0);
        }
    }

    void VulkanSceneRenderer::RecordSkyboxPass(RHICommandList& cmdList, const Scope<Cubemap>& cubemap) {
        auto& context = m_GraphicsDevice->GetContext();
        const auto currentFrame = context.GetCurrentFrameIndex();
        const auto& commandBuffer = context.GetCurrentFrame().commandBuffer;

        auto vk_Cubemap = static_cast<VulkanCubemap*>(cubemap.get());

        // Pipeline bind
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, vk_Cubemap->GetNativePipeline());

        // Identical to our record opaque pass code
        auto pipelineLayout = m_BindlessManager->GetPipelineLayout();
        auto bindlessSet = m_BindlessManager->GetDescriptorSet();
        auto& sceneSet = m_SceneDescriptorSets;
        // Descriptors bind
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, bindlessSet, nullptr);
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 1, *sceneSet[currentFrame], nullptr);

        // Push constant
        CubemapData pushData{};
        pushData.textureIndex = vk_Cubemap->GetTextureIndex();
        pushData.samplerIndex = vk_Cubemap->GetSamplerIndex();

        vk::ArrayProxy<const uint8_t> dataProxy(
            sizeof(CubemapData),
            reinterpret_cast<const uint8_t*>(&pushData)
        );
        commandBuffer.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, dataProxy);

        // Draw
        commandBuffer.draw(36, 1, 0, 0);
    }

}