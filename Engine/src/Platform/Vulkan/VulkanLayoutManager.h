#pragma once

#include "ZEngine/Renderer/LayoutManager.h"
#include <vulkan/vulkan_raii.hpp>

namespace ZEngine {

    namespace SetSlot {
        constexpr uint32_t Global = 0;
        constexpr uint32_t Pass = 1;
        constexpr uint32_t Material = 2;
        constexpr uint32_t Object = 3;
        constexpr uint32_t Count = 4;
    }

    class VulkanLayoutManager : public LayoutManager {
    public:
        VulkanLayoutManager();
        ~VulkanLayoutManager() = default;

        void Init() override;

        // Getters
        vk::PipelineLayout GetGlobalPipelineLayout() const { return *m_GlobalPipelineLayout; }
        vk::raii::PipelineLayout& GetRaiiPipelineLayout() { return m_GlobalPipelineLayout; }

        vk::DescriptorSetLayout GetDescriptorSetLayout(uint32_t slot) const { return m_RawLayouts.at(slot); }
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