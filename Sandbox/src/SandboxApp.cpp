#include "ZEngine.h"
#include "ZEngine/Core/EntryPoint.h"

#include <glm/gtc/matrix_transform.hpp>

#include "Sandbox2D.h"

class TestLayer : public ZEngine::Layer {
public:
	TestLayer()
		: Layer("Test"), m_CameraController(1280.0f / 720.0f)
	{
		// Shader
		m_Shader = ZEngine::Shader::Create("BistroShader", "BistroShader.spv");
		//m_Shader = ZEngine::Shader::Create("Shader", "Shader.spv");
		//auto m_Shader = m_ShaderLibrary.Load("Shader.spv");

		// Material and texture
		m_Texture = ZEngine::Texture2D::Create();
		//m_Texture->LoadTexture(ASSETS_DIR"/textures/shamrock_four.png");
		m_Texture->LoadTexture(ASSETS_DIR"/models/BistroModel/Textures/Shopsign_Book_Store_BaseColor.ktx2");
		m_MaterialInstance = ZEngine::Material::Create("Test Material");
		m_MaterialInstance->SetAlbedoTexture(m_Texture);
		m_MaterialInstance->Init();

		// Buffers and array config
		m_VertexArray = ZEngine::VertexArray::Create();
		float vertices[7 * 4] = {
			// Position   Color				TexCoords
			-0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
			 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
			 0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
			-0.5f,  0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f
		};

		uint32_t indices[6] = { 0, 1, 2, 2, 3, 0 };

		ZEngine::Ref<ZEngine::VertexBuffer> vertexBuffer;
		vertexBuffer = ZEngine::VertexBuffer::Create(vertices, sizeof(vertices));

		ZEngine::BufferLayout layout = {
			{ ZEngine::ShaderDataType::Float3, "position" },
			{ ZEngine::ShaderDataType::Float3, "normal" },
			{ ZEngine::ShaderDataType::Float2, "uv" }
		};
		
		ZEngine::BufferLayout layout1 = {
			{ ZEngine::ShaderDataType::Float2, "a_Position" },
			{ ZEngine::ShaderDataType::Float3, "a_Color" },
			{ ZEngine::ShaderDataType::Float2, "a_TexCoords" }
		};

		//vertexBuffer->SetLayout(layout);

		ZEngine::Ref<ZEngine::IndexBuffer> indexBuffer;
		indexBuffer = ZEngine::IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t));

		m_VertexArray->SetVertexBuffer(vertexBuffer);
		m_VertexArray->SetIndexBuffer(indexBuffer);

		// Pipeline state spec
		ZEngine::PipelineSpecification pipelineSpec{ m_Shader, layout, false, false };
		m_PipelineState = ZEngine::PipelineState::Create(pipelineSpec);

		// Model loading
		m_Model = ZEngine::Model::Create();
		//m_Model->LoadFromFile(ASSETS_DIR"/models/BistroModel/BistroInterior.gltf");
	}

	void OnUpdate(ZEngine::Timestep ts) override {
		// Update
		m_CameraController.OnUpdate(ts);

		// Render
		ZEngine::RenderCommand::SetViewport(0, 0, ZEngine::Application::Get().GetWindow().GetWidth(), ZEngine::Application::Get().GetWindow().GetHeight());
		ZEngine::RenderCommand::SetClearColor(glm::vec4(0.0f, 0.0f, 0.1f, 0.0f));

		glm::mat4 firstTransform = glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f, 0.0f, -1.0f));
		glm::mat4 secondTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -1.5f));

		ZEngine::Renderer::BeginScene(m_CameraController.GetCamera());
		//ZEngine::Renderer::Submit(m_PipelineState, m_MaterialInstance, firstTransform);
		//ZEngine::Renderer::Submit(m_PipelineState, m_MaterialInstance, secondTransform);
		//ZEngine::Renderer::DrawModel(m_Model, m_PipelineState);
		//ZEngine::Renderer::EndScene(m_VertexArray);
	}

	void OnEvent(ZEngine::Event& e) override {
		m_CameraController.OnEvent(e);
	}

private:
	ZEngine::ShaderLibrary m_ShaderLibrary;
	ZEngine::Ref<ZEngine::Shader> m_Shader;
	ZEngine::Ref<ZEngine::Texture2D> m_Texture;
	ZEngine::Ref<ZEngine::Material> m_MaterialInstance;
	ZEngine::Ref<ZEngine::VertexArray> m_VertexArray;
	ZEngine::Ref<ZEngine::PipelineState> m_PipelineState;
	ZEngine::Scope<ZEngine::Model> m_Model;

	ZEngine::PerspectiveCameraController m_CameraController;
	//ZEngine::OrthographicCameraController m_CameraController;
};

class Game : public ZEngine::Application {
public:
	Game() {
		PushLayer(new TestLayer());
		//PushLayer(new Sandbox2D());
;	}

	~Game() {

	}
};

ZEngine::Application* ZEngine::CreateApplication() {
	return new Game();
}