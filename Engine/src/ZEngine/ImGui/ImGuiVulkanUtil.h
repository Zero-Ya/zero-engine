#pragma once

#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanSwapchain.h"
#include "Platform/Vulkan/VulkanCommandManager.h"
#include "Platform/Vulkan/VulkanSyncManager.h"
#include "ZEngine/Renderer/ResourceManager.h"

#include <vulkan/vulkan_raii.hpp>
#include <imgui.h>
#include <glm/glm.hpp>

namespace ZEngine {
    class ImGuiVulkanUtil {
    private:
        // Buffers
        std::vector<vk::raii::Buffer>       vertexBuffers;
        std::vector<vk::raii::DeviceMemory> vertexBufferMemories;
        std::vector<vk::raii::Buffer>       indexBuffers;
        std::vector<vk::raii::DeviceMemory> indexBufferMemories;
        std::vector<uint32_t>               vertexCounts;
        std::vector<uint32_t>               indexCounts;

        // Vulkan resources
        vk::raii::DescriptorPool      descriptorPool      = nullptr;
        vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;
        vk::raii::DescriptorSet       descriptorSet       = nullptr;

        vk::raii::PipelineLayout      pipelineLayout      = nullptr;
        vk::raii::Pipeline            pipeline            = nullptr;

        vk::raii::Sampler             fontSampler         = nullptr;
        vk::raii::Image               fontImage           = nullptr;
        vk::raii::DeviceMemory        fontImageMemory     = nullptr;
        vk::raii::ImageView           fontImageView       = nullptr;

        // Vulkan device context and system integration
        VulkanContext*        vk_Ctx;
        VulkanSwapchain*      vk_Swapchain;
        VulkanCommandManager* vk_CommandManager;
        ResourceManager*      resourceManager;
        VulkanSyncManager*    vk_SyncManager;

        // UI state management and rendering configuration
        ImGuiStyle vulkanStyle;

        // Push constants
        struct PushConstBlock {
            glm::vec2 scale;
            glm::vec2 translate;
        } pushConstBlock{};

        // Modern Vulkan rendering configuration
        vk::PipelineRenderingCreateInfo renderingInfo;

    public:
        // Lifecycle management for proper resource initialization and cleanup
        ImGuiVulkanUtil(VulkanContext* ctx, VulkanSwapchain* swapchain, VulkanCommandManager* command, ResourceManager* resource, VulkanSyncManager* sync);
        ~ImGuiVulkanUtil();

        // Core functionality methods for ImGui integration
        void init(float width, float height);
        void initResources();
        void setStyle(uint32_t index);

        // Frame-by-frame rendering operations
        bool newFrame();
        void updateBuffers(uint32_t frameIndex);
        void drawFrame(vk::raii::CommandBuffer& commandBuffer, uint32_t imageIndex, uint32_t frameIndex);

        //void handleKey(int key, int scancode, int action, int mods);
        //bool getWantKeyCapture();
        //void charPressed(uint32_t key);

        void createPipeline();
        vk::raii::ShaderModule createShaderModule(const std::vector<char>& code) const;

    };
}