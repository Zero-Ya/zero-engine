#include "ZEngine.h"
#include <glm/gtc/matrix_transform.hpp>

class TestLayer : public ZEngine::Layer {
public:
	TestLayer()
		: Layer("Test"), m_Camera(), m_CameraPosition(0.0f)
	{
		auto& s_LayoutManager = ZEngine::Renderer::GetLayoutManager();
		auto& s_DescriptorAllocator = ZEngine::Renderer::GetDescriptorAllocator();

		// Camera
		m_Camera = ZEngine::PerspectiveCamera(45.0f, ZEngine::Application::Get().GetWindow().GetAspectRatio(), 0.1f, 10.0f);

		// Shader
		m_Shader = ZEngine::Shader::Create("Shader", "shader.spv");

		// Buffers and array config
		m_VertexArray = ZEngine::VertexArray::Create();
		float vertices[7 * 4] = {
			// Position   Color				TexCoords
			-0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
			 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
			 0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
			-0.5f,  0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f
		};

		uint32_t indices[6] = { 0, 1, 2, 2, 3, 0 };

		ZEngine::Ref<ZEngine::VertexBuffer> vertexBuffer;
		vertexBuffer = ZEngine::VertexBuffer::Create(vertices, sizeof(vertices));

		ZEngine::BufferLayout layout = {
			{ ZEngine::ShaderDataType::Float2, "a_Position" },
			{ ZEngine::ShaderDataType::Float3, "a_Color" },
			{ ZEngine::ShaderDataType::Float2, "a_TexCoords" }
		};
		vertexBuffer->SetLayout(layout);

		ZEngine::Ref<ZEngine::IndexBuffer> indexBuffer;
		indexBuffer = ZEngine::IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t));

		m_VertexArray->SetVertexBuffer(vertexBuffer);
		m_VertexArray->SetIndexBuffer(indexBuffer);

		// Pipeline state spec
		ZEngine::PipelineSpecification pipelineSpec{ m_Shader, layout, false, false };
		m_PipelineState = ZEngine::PipelineState::Create(pipelineSpec, ZEngine::Renderer::GetLayoutManager());

		// Material and texture
		m_Texture = ZEngine::Texture2D::Create("texture.jpg");
		m_MaterialInstance = ZEngine::Material::Create("Test Material", m_Texture);
		m_MaterialInstance->Init(s_DescriptorAllocator, s_LayoutManager);
	}

	void OnUpdate(ZEngine::Timestep ts) override {
		//ZE_TRACE("Delta time: {0}s ({1}ms)", ts.GetSeconds(), ts.GetMilliseconds());

		if (ZEngine::Input::IsKeyPressed(ZE_KEY_A))
			m_CameraPosition.x -= m_CameraMoveSpeed * ts;
		else if (ZEngine::Input::IsKeyPressed(ZE_KEY_D))
			m_CameraPosition.x += m_CameraMoveSpeed * ts;

		if (ZEngine::Input::IsKeyPressed(ZE_KEY_S))
			m_CameraPosition.y -= m_CameraMoveSpeed * ts;
		else if (ZEngine::Input::IsKeyPressed(ZE_KEY_W))
			m_CameraPosition.y += m_CameraMoveSpeed * ts;

		if (ZEngine::Input::IsKeyPressed(ZE_KEY_LEFT))
			m_CameraRotation += m_CameraRotationSpeed * ts;
		else if (ZEngine::Input::IsKeyPressed(ZE_KEY_RIGHT))
			m_CameraRotation -= m_CameraRotationSpeed * ts;

		ZEngine::RenderCommand::SetViewport(0, 0, ZEngine::Application::Get().GetWindow().GetWidth(), ZEngine::Application::Get().GetWindow().GetHeight());
		ZEngine::RenderCommand::SetClearColor(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));

		// Since order of operation matters, we must put rotation before position
		m_Camera.SetRotation(m_CameraRotation);
		m_Camera.SetPosition(m_CameraPosition);

		glm::mat4 triangleTransform = glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f, 0.0f, -1.0f));

		ZEngine::Renderer::BeginScene(m_Camera);
		ZEngine::Renderer::Submit(m_PipelineState, m_VertexArray, m_MaterialInstance, triangleTransform);
		ZEngine::Renderer::EndScene();
	}

	void OnEvent(ZEngine::Event& event) override {
	}

private:
	ZEngine::Ref<ZEngine::Shader> m_Shader;
	ZEngine::Ref<ZEngine::Texture2D> m_Texture;
	ZEngine::Ref<ZEngine::Material> m_MaterialInstance;
	ZEngine::Ref<ZEngine::VertexArray> m_VertexArray;
	ZEngine::Ref<ZEngine::PipelineState> m_PipelineState;

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