#include "Sandbox2D.h"

#include <imgui.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

Sandbox2D::Sandbox2D()
	: Layer("Sandbox2D"), m_CameraController(1280.0f / 720.0f)
{}

void Sandbox2D::OnAttach() {
	auto& s_LayoutManager = ZEngine::Renderer::GetLayoutManager();
	auto& s_DescriptorAllocator = ZEngine::Renderer::GetDescriptorAllocator();

	// Shader
	m_FlatShader = ZEngine::Shader::Create("Shader", "FlatShader.spv");

	// Buffers and array config
	m_SquareVertexArray = ZEngine::VertexArray::Create();
	float vertices[2 * 4] = {
		// Position
		-0.5f, -0.5f,
		 0.5f, -0.5f,
		 0.5f,  0.5f,
		-0.5f,  0.5f,
	};

	uint32_t indices[6] = { 0, 1, 2, 2, 3, 0 };

	ZEngine::Ref<ZEngine::VertexBuffer> vertexBuffer;
	vertexBuffer = ZEngine::VertexBuffer::Create(vertices, sizeof(vertices));

	ZEngine::BufferLayout layout = {
		{ ZEngine::ShaderDataType::Float2, "a_Position" },
	};
	vertexBuffer->SetLayout(layout);

	ZEngine::Ref<ZEngine::IndexBuffer> indexBuffer;
	indexBuffer = ZEngine::IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t));

	m_SquareVertexArray->SetVertexBuffer(vertexBuffer);
	m_SquareVertexArray->SetIndexBuffer(indexBuffer);

	// Pipeline state spec
	ZEngine::PipelineSpecification pipelineSpec{ m_FlatShader, layout, false, false };
	m_PipelineState = ZEngine::PipelineState::Create(pipelineSpec, ZEngine::Renderer::GetLayoutManager());
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

	ZEngine::Renderer::BeginScene(m_CameraController.GetCamera());
	ZEngine::Renderer::Submit(m_PipelineState, m_SquareVertexArray, nullptr, squareTransform);
	ZEngine::Renderer::EndScene();
}

void Sandbox2D::OnImGuiRender() {
	ImGui::Begin("Settings");
	ImGui::ColorEdit4("Square Color", glm::value_ptr(m_SquareColor));
	ImGui::End();
}

void Sandbox2D::OnEvent(ZEngine::Event& e) {
	m_CameraController.OnEvent(e);
}
