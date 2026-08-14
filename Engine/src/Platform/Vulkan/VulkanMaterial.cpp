#include "Platform/Vulkan/VulkanMaterial.h"
#include "Platform/Vulkan/VulkanTexture.h"

#include "ZEngine/Core/Application.h"
#include "Platform/Vulkan/VulkanContext.h"

namespace ZEngine {

    VulkanMaterial::VulkanMaterial(const std::string& name, const Ref<Texture2D>& texture)
        : m_Name(name), m_AlbedoTexture(texture)
    {
    }

    void VulkanMaterial::Init(const Scope<DescriptorAllocator>& descriptorAllocator, const Scope<LayoutManager>& layoutManager) {
        m_MaterialUBO = std::make_shared<VulkanUniformBuffer>(sizeof(MaterialProperties));
        AllocateDescriptorSet(descriptorAllocator, layoutManager);
        UpdateDescriptorSet();
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

    void VulkanMaterial::AllocateDescriptorSet(const Scope<DescriptorAllocator>& descriptorAllocator, const Scope<LayoutManager>& layoutManager) {
        auto vk_Allocator = static_cast<VulkanDescriptorAllocator*>(descriptorAllocator.get());
        m_MaterialSet = vk_Allocator->Allocate(SetSlot::Material, layoutManager);
    }

    void VulkanMaterial::UpdateDescriptorSet() {
        ZE_CORE_ASSERT(*m_MaterialSet != nullptr, "Material DescriptorSet has not been allocated!");

        auto vk_Context = static_cast<VulkanContext*>(Application::Get().GetGraphicsContext());
        auto& device = vk_Context->GetDevice();

        std::vector<vk::WriteDescriptorSet> writes;
        writes.reserve(2);

        // Binding 1: Material properties uniform buffer
        vk::DescriptorBufferInfo bufferInfo{ .buffer = m_MaterialUBO->GetUniformBuffers()[0], .offset = 0, .range = sizeof(MaterialProperties) };

        vk::WriteDescriptorSet uboWrite { .dstSet = *m_MaterialSet,
                                          .dstBinding = 0, // Set 2, Binding 0 (Material UBO)
                                          .dstArrayElement = 0,
                                          .descriptorCount = 1,
                                          .descriptorType = vk::DescriptorType::eUniformBuffer,
                                          .pBufferInfo = &bufferInfo
        };

        // Binding 0: Combined image sampler (Albedo texture)
        auto vulkanTexture = std::static_pointer_cast<VulkanTexture2D>(m_AlbedoTexture);
        vk::DescriptorImageInfo  imageInfo { .sampler = vulkanTexture->GetSampler(), .imageView = vulkanTexture->GetImageView(), .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal };

        vk::WriteDescriptorSet imageWrite { .dstSet = *m_MaterialSet,
                                            .dstBinding = 1, // Set 2, Binding 1 (Sampler)
                                            .dstArrayElement = 0,
                                            .descriptorCount = 1,
                                            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                                            .pImageInfo = &imageInfo
        };

        writes.push_back(imageWrite);
        writes.push_back(uboWrite);

        // Execute batch write
        device.updateDescriptorSets(writes, nullptr);
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