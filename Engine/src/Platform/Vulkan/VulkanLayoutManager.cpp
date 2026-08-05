#include "VulkanLayoutManager.h"

#include "ZEngine/Core/Application.h"
#include "VulkanContext.h"

namespace ZEngine {
    VulkanLayoutManager::VulkanLayoutManager() {
        Init();
    }

    void VulkanLayoutManager::Init() {
        auto vk_Context = static_cast<VulkanContext*>(Application::Get().GetGraphicsContext());
        auto& device = vk_Context->GetDevice();

        m_Layouts.reserve(SetSlot::Count);

        // Build individual fixed set layouts
        CreateGlobalSetLayout(device);
        CreatePassSetLayout(device);
        CreateMaterialSetLayout(device);
        CreateObjectSetLayout(device);

        // Cache raw unwrapped handles for C-style / struct initializations
        for (const auto& layout : m_Layouts) {
            m_RawLayouts.push_back(*layout);
        }

        vk::PipelineLayoutCreateInfo pipelineLayoutInfo { .setLayoutCount = 4, .pSetLayouts = m_RawLayouts.data(), .pushConstantRangeCount = 0};
        m_GlobalPipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);
    }

    // Set 0: Global frame data (Binding 0: Uniform buffer for view/projection/time)
    void VulkanLayoutManager::CreateGlobalSetLayout(const vk::raii::Device& device) {
        std::array bindings = {
            vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, nullptr),
        };

        vk::DescriptorSetLayoutCreateInfo layoutInfo{ .bindingCount = 1, .pBindings = bindings.data()};
        m_Layouts.emplace_back(vk::raii::DescriptorSetLayout(device, layoutInfo));
    }

    // Set 1: Pass data (Binding 0: Shadow map / G-Buffer sampler)
    void VulkanLayoutManager::CreatePassSetLayout(const vk::raii::Device& device){
        std::array bindings = {
            vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment, nullptr)
        };

        vk::DescriptorSetLayoutCreateInfo layoutInfo{ .bindingCount = 1, .pBindings = bindings.data() };
        m_Layouts.emplace_back(vk::raii::DescriptorSetLayout(device, layoutInfo));
    }

    // Set 2: Material data (Binding 0: Material UBO params, Binding 1: Albedo texture)
    void VulkanLayoutManager::CreateMaterialSetLayout(const vk::raii::Device& device) {
        std::array bindings = {
            // Binding 0: Material properties (Tint color, roughness, metallic, etc.)
            vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment, nullptr),
            // Binding 1: Main albedo combined image sampler
            vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment, nullptr)
        };

        vk::DescriptorSetLayoutCreateInfo layoutInfo{ .bindingCount = 1, .pBindings = bindings.data() };
        m_Layouts.emplace_back(vk::raii::DescriptorSetLayout(device, layoutInfo));
    }

    // Set 3: Object data (Binding 0: Per-draw model matrix UBO)
    void VulkanLayoutManager::CreateObjectSetLayout(const vk::raii::Device& device) {
        std::array bindings = {
            vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex, nullptr)
        };

        vk::DescriptorSetLayoutCreateInfo layoutInfo{ .bindingCount = 1, .pBindings = bindings.data() };
        m_Layouts.emplace_back(vk::raii::DescriptorSetLayout(device, layoutInfo));
    }

}