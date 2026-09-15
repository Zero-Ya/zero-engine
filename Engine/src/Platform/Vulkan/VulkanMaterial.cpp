#include "Platform/Vulkan/VulkanMaterial.h"
#include "Platform/Vulkan/VulkanTexture.h"

#include "ZEngine/Core/Application.h"
#include "Platform/Vulkan/VulkanContext.h"

#include "Platform/Vulkan/VulkanBuffer.h"
#include "Platform/Vulkan/VulkanDescriptorAllocator.h"

namespace ZEngine {

    VulkanMaterial::VulkanMaterial(const std::string& name)
        : m_Name(name)
    {}

    void VulkanMaterial::Init() {
        m_MaterialUBO = UniformBuffer::Create(sizeof(MaterialProperties));
        //AllocateDescriptorSet();
        //UpdateDescriptorSet();
    }

    void VulkanMaterial::SetAlbedoColor(const glm::vec4& color) {
        m_Properties.Albedo = color;
        m_IsDirty = true;
    }

    void VulkanMaterial::SetRoughness(float roughness) {
        m_Properties.Roughness = roughness;
        m_IsDirty = true;
    }

    void VulkanMaterial::SetMetallic(float metallic) {
        m_Properties.Metallic = metallic;
        m_IsDirty = true;
    }

    void VulkanMaterial::SetAlbedoTexture(Ref<Texture2D> texture) {
        m_AlbedoTexture = texture;
        m_IsDirty = true;
    }

    void VulkanMaterial::AllocateDescriptorSet() {
        auto vk_Context = static_cast<VulkanContext*>(Application::Get().GetGraphicsContext());

        auto vk_Allocator = static_cast<VulkanDescriptorAllocator*>(vk_Context->GetDescriptorAllocator().get());
        m_MaterialSet = vk_Allocator->Allocate(SetSlot::Material);
    }

    void VulkanMaterial::UpdateDescriptorSets() {
        ZE_CORE_ASSERT(*m_MaterialSet != nullptr, "Material DescriptorSet has not been allocated!");

        auto vk_Context = static_cast<VulkanContext*>(Application::Get().GetGraphicsContext());
        auto& device = vk_Context->GetDevice();

        // Binding 0: Material properties uniform buffer
        const auto& materialUBO = static_cast<VulkanUniformBuffer*>(m_MaterialUBO.get());
        vk::DescriptorBufferInfo bufferInfo{ .buffer = materialUBO->GetUniformBuffers()[0], .offset = 0, .range = sizeof(MaterialProperties) };

        vk::WriteDescriptorSet uboWrite { .dstSet = *m_MaterialSet,
                                          .dstBinding = 0, // Set 2, Binding 0 (Material UBO)
                                          .dstArrayElement = 0,
                                          .descriptorCount = 1,
                                          .descriptorType = vk::DescriptorType::eUniformBuffer,
                                          .pBufferInfo = &bufferInfo
        };
        device.updateDescriptorSets(uboWrite, {});

        // Binding 1: Combined image sampler (Albedo texture)
        auto vulkanTexture = std::static_pointer_cast<VulkanTexture2D>(m_AlbedoTexture);
        vulkanTexture->UpdateDescriptorSet(m_MaterialSet);
    }

    void VulkanMaterial::UpdateBuffer() {
        if (m_MaterialUBO) {
            // Only update data if property was modified
            if (m_IsDirty) {
                m_MaterialUBO->SetData(&m_Properties, sizeof(MaterialProperties));
                m_IsDirty = false;
            }
        }
    }

}