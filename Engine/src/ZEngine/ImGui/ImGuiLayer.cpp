#include "ImGuiLayer.h"
#include <imgui.h>
#include <GLFW/glfw3.h>

#include "ZEngine/Core/Application.h"
#include "Platform/Vulkan/VulkanImGuiUtil.h"

namespace ZEngine {

    ImGuiLayer::ImGuiLayer() : Layer("ImGuiLayer") {}

    void ImGuiLayer::OnAttach() {
        m_Backend = std::make_unique<VulkanImGuiUtil>();

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        ImGui::StyleColorsDark();

        GLFWwindow* nativeWindow = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
        m_Backend.get()->Init(nativeWindow);
    }

    void ImGuiLayer::OnDetach() {
        m_Backend.get()->Shutdown();
        ImGui::DestroyContext();
    }

    void ImGuiLayer::OnImGuiRender() {
        if (m_ShowDemo)
            ImGui::ShowDemoWindow(&m_ShowDemo);
    }

    void ImGuiLayer::Begin() {
        m_Backend.get()->BeginFrame();
    }

    void ImGuiLayer::End(const Ref<RenderCommandBuffer>& renderCommandBuffer) {
        m_Backend.get()->EndFrame(renderCommandBuffer);
    }

}