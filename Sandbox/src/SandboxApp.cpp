#include "ZEngine.h"
#include "ZEngine/Core/EntryPoint.h"

#include <glm/gtc/matrix_transform.hpp>

#include "Sandbox2D.h"

class TestLayer : public ZEngine::Layer {
public:
	TestLayer()
		: Layer("Test"), m_CameraController(1280.0f / 720.0f)
	{
		auto& app = ZEngine::Application::Get();
		auto& graphicsDevice = app.GetGraphicsDevice();
		auto& commandList = graphicsDevice->GetMainCommandList();

		auto& bindlessManager = app.GetBindlessManager();
		auto& resourceManager = app.GetResourceManager();

		commandList.SetResourceManager(resourceManager);

		// Render graph
		m_RenderGraph = std::make_unique<ZEngine::RenderGraph>();

		// Model loader
		m_ModelLoader = ZEngine::ModelLoader::Create(graphicsDevice, resourceManager, bindlessManager);

		// Scene renderer
		m_SceneRenderer = ZEngine::SceneRenderer::Create(graphicsDevice, resourceManager, bindlessManager);

		// Create swapchain texture and depth handle
		m_SwapchainTextureHandle = ZEngine::TextureHandle::MakeExternal(ZEngine::SWAPCHAIN_TEXTURE_ID);
		m_SwapchainDepthHandle = ZEngine::TextureHandle::MakeExternal(ZEngine::SWAPCHAIN_DEPTH_ID);

		// Shader
		auto m_BistroShader = m_ShaderLibrary.Load("Model Shader", "ModelShader.spv");
		auto m_SkyboxShader = m_ShaderLibrary.Load("Skybox Shader", "SkyboxShader.spv");

		// Model layout for pipeline
		ZEngine::BufferLayout modelLayout = {
			{ ZEngine::ShaderDataType::Float3, "position" },
			{ ZEngine::ShaderDataType::Float3, "normal" },
			{ ZEngine::ShaderDataType::Float2, "uv" }
		};

		// Model pipeline state
		ZEngine::PipelineSpecification modelSpec { m_BistroShader, modelLayout, false, false };
		m_ModelPipeline = ZEngine::PipelineState::Create(modelSpec);

		// Skybox pipeline state
		ZEngine::PipelineSpecification skyboxSpec { m_SkyboxShader, {}, false, false };

		// Model loading
		//m_Model = m_ModelLoader->LoadModel(ASSETS_DIR"/models/SponzaModel/Sponza.gltf").value();
		m_Model = m_ModelLoader->LoadModel(ASSETS_DIR"/models/BistroModel/BistroExterior.gltf").value();

		// Skybox
		std::vector<std::string> skyboxTextures = {
			ASSETS_DIR"/textures/skybox/right.jpg",
			ASSETS_DIR"/textures/skybox/left.jpg",
			ASSETS_DIR"/textures/skybox/top.jpg",
			ASSETS_DIR"/textures/skybox/bottom.jpg",
			ASSETS_DIR"/textures/skybox/front.jpg",
			ASSETS_DIR"/textures/skybox/back.jpg",
		};
		m_Cubemap = ZEngine::Cubemap::Create(graphicsDevice, bindlessManager, resourceManager);
		m_Cubemap->LoadCubemap(skyboxTextures);
		m_Cubemap->Init(skyboxSpec);
	}

	void OnUpdate(ZEngine::Timestep ts) override {
		// Update
		m_CameraController.OnUpdate(ts);

		// Render
		auto& graphicsDevice = ZEngine::Application::Get().GetGraphicsDevice();
		auto& commandList = graphicsDevice->GetMainCommandList();
		auto& resourceManager = ZEngine::Application::Get().GetResourceManager();

		resourceManager->ImportExternalResources();
		m_RenderGraph->Clear();

		m_SceneRenderer->BeginFrame();
		m_SceneRenderer->SubmitModel(m_Model, glm::mat4(1.0f));

		// Main pass
		m_RenderGraph->AddPass(
			"MainPass",
			[&](ZEngine::RenderGraphPassBuilder& builder) {
				builder.WriteColor(m_SwapchainTextureHandle);
				builder.WriteDepth(m_SwapchainDepthHandle);
			},
			[&](ZEngine::RHICommandList& command) {
				ZEngine::ColorAttachmentInfo colorAttachmentInfo {
					.texture = m_SwapchainTextureHandle,
					.layout = ZEngine::TextureLayout::COLOR_ATTACHMENT
				};

				ZEngine::RenderingInfo renderingInfo {
					.renderWidth = ZEngine::Application::Get().GetWindow().GetWidth(),
					.renderHeight = ZEngine::Application::Get().GetWindow().GetHeight(),
					.colorAttachments = std::vector(1, colorAttachmentInfo),
					.colorAttachmentCount = 1,
					.depthAttachment = m_SwapchainDepthHandle,
					.hasDepth = true
				};

				command.BeginRendering(renderingInfo);

				command.SetViewport(0, 0, ZEngine::Application::Get().GetWindow().GetWidth(), ZEngine::Application::Get().GetWindow().GetHeight());
				command.SetScissor(0, 0, ZEngine::Application::Get().GetWindow().GetWidth(), ZEngine::Application::Get().GetWindow().GetHeight());
				// SetClearColor

				m_SceneRenderer->UpdateSceneBuffers(m_CameraController.GetCamera());
				m_SceneRenderer->SetPipeline(m_ModelPipeline);
				m_SceneRenderer->RecordOpaquePass(command);
				m_SceneRenderer->RecordSkyboxPass(command, m_Cubemap);

				// ImGui (Temporary)
				auto imGuiLayer = ZEngine::Application::Get().GetImGuiLayer();
				imGuiLayer->Begin();
				imGuiLayer->OnImGuiRender();
				imGuiLayer->End();

				command.EndRendering();
			}
		);

		// Final pass: Present-to-image pass
		m_RenderGraph->AddPass(
			"PresentPass",
			[&](ZEngine::RenderGraphPassBuilder& builder) {
				builder.Read(m_SwapchainTextureHandle, ZEngine::TextureLayout::PRESENT_SRC);
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
	ZEngine::Ref<ZEngine::PipelineState> m_ModelPipeline;
	ZEngine::Model m_Model;

	ZEngine::TextureHandle m_SwapchainTextureHandle;
	ZEngine::TextureHandle m_SwapchainDepthHandle;

	ZEngine::Scope<ZEngine::ModelLoader> m_ModelLoader;
	ZEngine::Scope<ZEngine::Cubemap> m_Cubemap;
	ZEngine::Scope<ZEngine::RenderGraph> m_RenderGraph;
	ZEngine::Scope<ZEngine::SceneRenderer> m_SceneRenderer;

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