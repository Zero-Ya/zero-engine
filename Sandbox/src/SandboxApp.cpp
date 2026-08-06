#include "ZEngine.h"
#include <glm/gtc/matrix_transform.hpp>

class TestLayer : public ZEngine::Layer {
public:
	TestLayer()
		: Layer("Test"), m_Camera(), m_CameraPosition(0.0f)
	{
		// Camera
		m_Camera = ZEngine::PerspectiveCamera(45.0f, ZEngine::Application::Get().GetWindow().GetAspectRatio(), 0.1f, 10.0f);

		// Shader
		m_Shader = ZEngine::Shader::Create("Shader", "shader.spv");

		// Buffers and array config
		m_VertexArray = ZEngine::VertexArray::Create();
		float vertices[5 * 4] = {
			// Position   Color
			-0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
			 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
			 0.5f,  0.5f, 0.0f, 0.0f, 1.0f,
			-0.5f,  0.5f, 1.0f, 1.0f, 1.0f
		};

		uint32_t indices[6] = { 0, 1, 2, 2, 3, 0 };

		std::shared_ptr<ZEngine::VertexBuffer> vertexBuffer;
		vertexBuffer = ZEngine::VertexBuffer::Create(vertices, sizeof(vertices));

		ZEngine::BufferLayout layout = {
			{ ZEngine::ShaderDataType::Float2, "a_Position" },
			{ ZEngine::ShaderDataType::Float3, "a_Color" }
		};
		vertexBuffer->SetLayout(layout);

		std::shared_ptr<ZEngine::IndexBuffer> indexBuffer;
		indexBuffer = ZEngine::IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t));

		m_VertexArray->SetVertexBuffer(vertexBuffer);
		m_VertexArray->SetIndexBuffer(indexBuffer);

		// Pipeline state spec
		ZEngine::PipelineSpecification pipelineSpec{ m_Shader, layout, false, false };
		m_PipelineState = ZEngine::PipelineState::Create(pipelineSpec, ZEngine::Renderer::GetLayoutManager());
	}

	void OnUpdate(ZEngine::Timestep ts) override {

		//ZE_TRACE("Delta time: {0}s ({1}ms)", ts.GetSeconds(), ts.GetMilliseconds());

		if (ZEngine::Input::IsKeyPressed(ZE_KEY_LEFT))
			m_CameraPosition.x -= m_CameraMoveSpeed * ts;
		else if (ZEngine::Input::IsKeyPressed(ZE_KEY_RIGHT))
			m_CameraPosition.x += m_CameraMoveSpeed * ts;

		if (ZEngine::Input::IsKeyPressed(ZE_KEY_DOWN))
			m_CameraPosition.y -= m_CameraMoveSpeed * ts;
		else if (ZEngine::Input::IsKeyPressed(ZE_KEY_UP))
			m_CameraPosition.y += m_CameraMoveSpeed * ts;

		if (ZEngine::Input::IsKeyPressed(ZE_KEY_A))
			m_CameraRotation += m_CameraRotationSpeed * ts;
		else if (ZEngine::Input::IsKeyPressed(ZE_KEY_D))
			m_CameraRotation -= m_CameraRotationSpeed * ts;

		ZEngine::RenderCommand::SetViewport(0, 0, ZEngine::Application::Get().GetWindow().GetWidth(), ZEngine::Application::Get().GetWindow().GetHeight());
		ZEngine::RenderCommand::SetClearColor(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));

		// Since order of operation matters, we must put rotation before position
		m_Camera.SetRotation(m_CameraRotation);
		m_Camera.SetPosition(m_CameraPosition);

		glm::mat4 triangleTransform = glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f, 0.0f, -1.0f));

		ZEngine::Renderer::BeginScene(m_Camera);
		ZEngine::Renderer::Submit(m_PipelineState, m_VertexArray, triangleTransform);
		ZEngine::Renderer::EndScene();
	}

	void OnEvent(ZEngine::Event& event) override {
	}

private:
	std::shared_ptr<ZEngine::Shader> m_Shader;
	std::shared_ptr<ZEngine::VertexArray> m_VertexArray;
	std::shared_ptr<ZEngine::PipelineState> m_PipelineState;

	ZEngine::PerspectiveCamera m_Camera;
	glm::vec3 m_CameraPosition;
	float m_CameraMoveSpeed = 10.0f;
	float m_CameraRotation = 0.0f;
	float m_CameraRotationSpeed = 90.0f;
};

class Game : public ZEngine::Application {
public:
	Game() {
		PushLayer(new TestLayer());
	}

	~Game() {

	}
};

ZEngine::Application* ZEngine::CreateApplication() {
	return new Game();
}