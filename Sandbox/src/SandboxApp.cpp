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
		m_Model->LoadFromFile(ASSETS_DIR"/models/BistroModel/BistroExterior.gltf");

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

		// Render graph
		m_RenderGraph = std::make_unique<ZEngine::RenderGraph>();

	}

	void OnUpdate(ZEngine::Timestep ts) override {
		// Update
		m_CameraController.OnUpdate(ts);

		// Render
		auto& graphicsDevice = ZEngine::Application::Get().GetGraphicsDevice();
		auto& commandList = graphicsDevice->GetMainCommandList();

		glm::mat4 firstTransform = glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f, 0.0f, -1.0f));
		glm::mat4 secondTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -1.5f));

		// Handles for render graph
		ZEngine::TextureHandle swapchainTexture = graphicsDevice->GetSwapchainTextureHandle();
		ZEngine::TextureHandle depthTexture = graphicsDevice->GetSwapchainDepthHandle();

		m_RenderGraph->Clear();

		// Pass 1: Main pass
		m_RenderGraph->AddPass(
			"MainPass",
			[&](ZEngine::RenderGraphPassBuilder& builder) {
				builder.WriteColor(swapchainTexture);
				builder.WriteDepth(depthTexture);
			},
			[&](ZEngine::RHICommandList& command) {
				command.BeginRendering(swapchainTexture, depthTexture);

				commandList.SetViewport(0, 0, ZEngine::Application::Get().GetWindow().GetWidth(), ZEngine::Application::Get().GetWindow().GetHeight());
				commandList.SetScissor(0, 0, ZEngine::Application::Get().GetWindow().GetWidth(), ZEngine::Application::Get().GetWindow().GetHeight());

				//ZEngine::RenderCommand::SetViewport(0, 0, ZEngine::Application::Get().GetWindow().GetWidth(), ZEngine::Application::Get().GetWindow().GetHeight());
				ZEngine::RenderCommand::SetClearColor(glm::vec4(0.0f, 0.0f, 0.1f, 0.0f));

				ZEngine::Renderer::BeginScene(m_CameraController.GetCamera());
				ZEngine::Renderer::GlobalBegin(m_ModelPipeline);
				//ZEngine::Renderer::GlobalBegin(m_DefaultPipeline);
				//ZEngine::Renderer::MeshBegin(m_DefaultPipeline, m_MaterialInstance, secondTransform);
				ZEngine::Renderer::DrawModel(m_Model, m_ModelPipeline); // Needs to be changed later
				//ZEngine::Renderer::DrawMesh(m_VertexArray);
				ZEngine::Renderer::DrawSkybox(m_Skybox);
				ZEngine::Renderer::EndScene();

				// ImGui (Temporary)
				auto imGuiLayer = ZEngine::Application::Get().GetImGuiLayer();
				imGuiLayer->Begin();
				imGuiLayer->OnImGuiRender();
				imGuiLayer->End();

				command.EndRendering();
			}
		);

		// Pass 2: ImGui pass
		//m_RenderGraph->AddPass(
		//	"ImGuiPass",
		//	[&](ZEngine::RenderGraphPassBuilder& builder) {
		//		builder.WriteColor(swapchainTexture);
		//	},
		//	[&](ZEngine::RHICommandList& command) {
		//		command.BeginRendering(swapchainTexture, depthTexture);

		//		auto imGuiLayer = ZEngine::Application::Get().GetImGuiLayer();

		//		imGuiLayer->Begin();
		//		imGuiLayer->OnImGuiRender();
		//		imGuiLayer->End();

		//		command.EndRendering();
		//	}
		//);

		// Final pass: Present-to-image pass (Is it always final?)
		m_RenderGraph->AddPass(
			"PresentPass",
			[&](ZEngine::RenderGraphPassBuilder& builder) {
				builder.Read(swapchainTexture, ZEngine::TextureLayout::PRESENT_SRC);
			},
			[&](ZEngine::RHICommandList& command) {
				// No commands needed
			}
		);

		// Compile and execute graph (No compile yet)
		m_RenderGraph->Execute(commandList);

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

	ZEngine::Scope<ZEngine::RenderGraph> m_RenderGraph;

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