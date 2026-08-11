#include "VulkanImGuiUtil.h"
#include "ZEngine/Core/Application.h"
#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanSwapchain.h"
#include "Platform/Vulkan/VulkanCommandBuffer.h"

#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_glfw.h>

namespace ZEngine {

	void VulkanImGuiUtil::Init(GLFWwindow* window) {
        auto vk_Context = static_cast<VulkanContext*>(Application::Get().GetGraphicsContext());
        auto vk_Swapchain = vk_Context->GetSwapchain();
        VkFormat colorAttachmentFormat = static_cast<VkFormat>(vk_Swapchain->GetSurfaceFormat().format);

        ImGui_ImplGlfw_InitForVulkan(window, true);

        ImGui_ImplVulkan_InitInfo initInfo = {};
        initInfo.Instance = *vk_Context->GetInstance();
        initInfo.PhysicalDevice = *vk_Context->GetPhysicalDevice();
        initInfo.Device = *vk_Context->GetDevice();
        initInfo.QueueFamily = vk_Context->GetQueueIndex();
        initInfo.Queue = *vk_Context->GetGraphicsQueue();
        initInfo.DescriptorPool = *m_DescriptorPool;
        initInfo.MinImageCount = vk_Swapchain->GetMinImageCount();
        initInfo.ImageCount = vk_Swapchain->GetImageCount();
        initInfo.DescriptorPoolSize = 1000;

        // For dynamic rendering
        initInfo.UseDynamicRendering = true;
        initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
        initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
        initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
        initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &colorAttachmentFormat;
        // Might need more later

        bool success = ImGui_ImplVulkan_Init(&initInfo);
        ZE_CORE_ASSERT(success, "Failed to initialize ImGui Vulkan backend pipeline!");

        CreateDescriptorPool();
	}

    void VulkanImGuiUtil::SetStyle() {

    }

    void VulkanImGuiUtil::CreateDescriptorPool() {
        auto vk_Context = static_cast<VulkanContext*>(Application::Get().GetGraphicsContext());

        std::array poolSize {
            vk::DescriptorPoolSize(vk::DescriptorType::eSampler, 1000),
            vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1000),
            vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, 1000),
            vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 1000),
            vk::DescriptorPoolSize(vk::DescriptorType::eUniformTexelBuffer, 1000),
            vk::DescriptorPoolSize(vk::DescriptorType::eStorageTexelBuffer, 1000),
            vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1000),
            vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 1000),
            vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 1000),
            vk::DescriptorPoolSize(vk::DescriptorType::eStorageBufferDynamic, 1000),
            vk::DescriptorPoolSize(vk::DescriptorType::eInputAttachment, 1000)
        };

        vk::DescriptorPoolCreateInfo poolInfo {
            .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            .maxSets = 1000 * poolSize.size(),
            .poolSizeCount = static_cast<uint32_t>(poolSize.size()),
            .pPoolSizes = poolSize.data()
        };

        m_DescriptorPool = vk::raii::DescriptorPool(vk_Context->GetDevice(), poolInfo);
    }

    void VulkanImGuiUtil::Shutdown() {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
    }

    void VulkanImGuiUtil::BeginFrame() {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void VulkanImGuiUtil::EndFrame(const Ref<RenderCommandBuffer>& renderCommandBuffer) {
        auto vk_Context = static_cast<VulkanContext*>(Application::Get().GetGraphicsContext());
        auto vulkanCommandBuffer = static_cast<VulkanCommandBuffer*>(renderCommandBuffer.get());
        const auto& commandBuffer = vulkanCommandBuffer->GetBuffer();

        ImGui::Render();
        ImDrawData* drawData = ImGui::GetDrawData();

        // Record ImGui rendering into active command buffer
        ImGui_ImplVulkan_RenderDrawData(drawData, *commandBuffer);

        // Update secondary viewports if docking/viewports are enabled
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            // Save context state
            GLFWwindow* backupCurrentContext = glfwGetCurrentContext();

            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();

            vk_Context->QueueWaitIdle();
            // Restore context state
            glfwMakeContextCurrent(backupCurrentContext);
        }
    }

}