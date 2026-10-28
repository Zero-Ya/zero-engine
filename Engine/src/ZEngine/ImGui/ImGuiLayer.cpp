#include "ImGuiLayer.h"

#include <imgui.h>
#include <GLFW/glfw3.h>
#include <glm/ext/matrix_transform.hpp>

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

    void ImGuiLayer::OnImGuiRender(DirectionalLight& directionalLight) {
        //if (m_ShowDemo)
            //ImGui::ShowDemoWindow(&m_ShowDemo);

        ImGui::Begin("Directional Light Controls");

        // Display current direction vector
        ImGui::Text("Sun Direction: (%.2f, %.2f, %.2f)",
            directionalLight.direction.x,
            directionalLight.direction.y,
            directionalLight.direction.z);

        // Manual sliders for testing
        static float sunAngle = 0.0f;
        if (ImGui::SliderFloat("Sun Orbit Angle", &sunAngle, -1.0f, 1.0f)) {
            glm::mat4 rot = glm::rotate(glm::mat4(1.0f), sunAngle, glm::vec3(1.0f, 0.0f, 0.0f)); // Rotate around X-axis
            directionalLight.direction = glm::normalize(glm::vec3(rot * glm::vec4(0.2f, -1.0f, -0.5f, 0.0f)));
        }

        ImGui::ColorEdit3("Sun Color", &directionalLight.color.x);
        ImGui::DragFloat("Sun Intensity", &directionalLight.intensity, 0.1f, 0.0f, 10.0f);

        ImGui::End();
    }

    void ImGuiLayer::Begin() {
        m_Backend.get()->BeginFrame();
    }

    void ImGuiLayer::End() {
        m_Backend.get()->EndFrame();
    }

}