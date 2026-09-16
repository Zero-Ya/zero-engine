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
		auto m_DefaultShader = m_ShaderLibrary.Load("Default Shader", "Shader.spv");
		auto m_BistroShader = m_ShaderLibrary.Load("Bistro Shader", "BistroShader.spv");
		auto m_SkyboxShader = m_ShaderLibrary.Load("Skybox Shader", "SkyboxShader.spv");

		// Material and texture (We're imitating a model instance since we don't have a mesh class yet...)
		m_Texture = ZEngine::Texture2D::Create();
		//m_Texture->LoadTexture(ASSETS_DIR"/textures/shamrock_four.png");
		m_Texture->LoadTexture(ASSETS_DIR"/models/BistroModel/Textures/Shopsign_Book_Store_BaseColor.ktx2");
		m_MaterialInstance = ZEngine::Material::Create("Test Material");
		m_MaterialInstance->SetAlbedoTexture(m_Texture);
		m_MaterialInstance->Init();
		m_MaterialInstance->AlloUpdateSet();

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

		ZEngine::BufferLayout defaultLayout = {
			{ ZEngine::ShaderDataType::Float2, "a_Position" },
			{ ZEngine::ShaderDataType::Float3, "a_Color" },
			{ ZEngine::ShaderDataType::Float2, "a_TexCoords" }
		};

		ZEngine::BufferLayout modelLayout = {
			{ ZEngine::ShaderDataType::Float3, "position" },
			{ ZEngine::ShaderDataType::Float3, "normal" },
			{ ZEngine::ShaderDataType::Float2, "uv" }
		};

		//vertexBuffer->SetLayout(layout);

		ZEngine::Ref<ZEngine::IndexBuffer> indexBuffer;
		indexBuffer = ZEngine::IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t));

		m_VertexArray->SetVertexBuffer(vertexBuffer);
		m_VertexArray->SetIndexBuffer(indexBuffer);

		// Default pipeline state
		ZEngine::PipelineSpecification defaultSpec{ m_DefaultShader, defaultLayout, false, false };
		m_DefaultPipeline = ZEngine::PipelineState::Create(defaultSpec);

		// Model pipeline state
		ZEngine::PipelineSpecification modelSpec{ m_BistroShader, modelLayout, false, false };
		m_ModelPipeline = ZEngine::PipelineState::Create(modelSpec);

		// Skybox pipeline state
		ZEngine::PipelineSpecification skyboxSpec{ m_SkyboxShader, {}, false, false };

		// Model loading
		m_Model = ZEngine::Model::Create();
		//m_Model->LoadFromFile(ASSETS_DIR"/models/BistroModel/BistroExterior.gltf");

		// Skybox
		std::vector<std::string> skyboxTextures = {
			ASSETS_DIR"/textures/skybox/right.jpg",
			ASSETS_DIR"/textures/skybox/left.jpg",
			ASSETS_DIR"/textures/skybox/top.jpg",
			ASSETS_DIR"/textures/skybox/bottom.jpg",
			ASSETS_DIR"/textures/skybox/front.jpg",
			ASSETS_DIR"/textures/skybox/back.jpg",
		};

		m_Skybox = ZEngine::Cubemap::Create();
		m_Skybox->LoadCubemap(skyboxTextures);
		m_Skybox->Init(skyboxSpec);
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
		ZEngine::Renderer::GlobalBegin(m_ModelPipeline);
		//ZEngine::Renderer::GlobalBegin(m_DefaultPipeline);
		//ZEngine::Renderer::MeshBegin(m_DefaultPipeline, m_MaterialInstance, secondTransform);
		//ZEngine::Renderer::DrawModel(m_Model, m_ModelPipeline); // Needs to be changed later
		//ZEngine::Renderer::DrawMesh(m_VertexArray);
		ZEngine::Renderer::DrawSkybox(m_Skybox);
		ZEngine::Renderer::EndScene();
	}

	void OnEvent(ZEngine::Event& e) override {
		m_CameraController.OnEvent(e);
	}

private:
	ZEngine::ShaderLibrary m_ShaderLibrary;
	ZEngine::Ref<ZEngine::Texture2D> m_Texture;
	ZEngine::Ref<ZEngine::Material> m_MaterialInstance;
	ZEngine::Ref<ZEngine::VertexArray> m_VertexArray;
	ZEngine::Ref<ZEngine::PipelineState> m_DefaultPipeline;
	ZEngine::Ref<ZEngine::PipelineState> m_ModelPipeline;
	ZEngine::Scope<ZEngine::Model> m_Model;
	ZEngine::Scope<ZEngine::Cubemap> m_Skybox;

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