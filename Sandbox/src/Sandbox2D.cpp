#include "Sandbox2D.h"

#include <imgui.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

Sandbox2D::Sandbox2D()
	: Layer("Sandbox2D"), m_CameraController(1280.0f / 720.0f)
{}

void Sandbox2D::OnAttach() {

}

void Sandbox2D::OnDetach() {

}

void Sandbox2D::OnUpdate(ZEngine::Timestep ts) {
	// Update
	m_CameraController.OnUpdate(ts);

	// Render
	ZEngine::RenderCommand::SetViewport(0, 0, ZEngine::Application::Get().GetWindow().GetWidth(), ZEngine::Application::Get().GetWindow().GetHeight());
	ZEngine::RenderCommand::SetClearColor(glm::vec4(0.0f, 0.0f, 0.1f, 0.0f));

	glm::mat4 squareTransform = glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f, 0.0f, -1.0f));

	ZEngine::Renderer2D::BeginScene(m_CameraController.GetCamera());
	ZEngine::Renderer2D::DrawQuad({0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f});
	ZEngine::Renderer2D::EndScene();
}

void Sandbox2D::OnImGuiRender() {
	ImGui::Begin("Settings");
	ImGui::ColorEdit4("Square Color", glm::value_ptr(m_SquareColor));
	ImGui::End();
}

void Sandbox2D::OnEvent(ZEngine::Event& e) {
	m_CameraController.OnEvent(e);
}
