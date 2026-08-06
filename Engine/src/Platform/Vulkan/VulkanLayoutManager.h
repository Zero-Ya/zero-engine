#pragma once

#include "ZEngine/Renderer/LayoutManager.h"
#include <vulkan/vulkan_raii.hpp>

#include <glm/glm.hpp>

namespace ZEngine {

    struct PushConstantData {
        glm::mat4 transform { 1.0f };
    };

    class VulkanLayoutManager : public LayoutManager {
    public:
        VulkanLayoutManager();
        ~VulkanLayoutManager() = default;

        void Init() override;

        // Getters
        vk::PipelineLayout GetGlobalPipelineLayout() const { return *m_GlobalPipelineLayout; }
        vk::raii::PipelineLayout& GetRaiiPipelineLayout() { return m_GlobalPipelineLayout; }

        vk::DescriptorSetLayout GetSetLayout(SetSlot slot) const { return *m_Layouts[static_cast<size_t>(slot)]; }
        std::vector<vk::DescriptorSetLayout>& GetRawLayouts() { return m_RawLayouts; }

    private:
        void CreateGlobalSetLayout(const vk::raii::Device& device);
        void CreatePassSetLayout(const vk::raii::Device& device);
        void CreateMaterialSetLayout(const vk::raii::Device& device);
        void CreateObjectSetLayout(const vk::raii::Device& device);

    private:
        // RAII descriptor set layouts
        std::vector<vk::raii::DescriptorSetLayout> m_Layouts;
        // Raw descriptor set layouts
        std::vector<vk::DescriptorSetLayout> m_RawLayouts;

        vk::raii::PipelineLayout m_GlobalPipelineLayout = nullptr;
    };

}