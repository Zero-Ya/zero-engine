#include "Application.h"
#include "Log.h"
#include "Input.h"

#include "ZEngine/Renderer/Renderer.h"
#include "ZEngine/Renderer/PerspectiveCamera.h"

namespace ZEngine {

#define BIND_EVENT_FN(x) std::bind(&Application::x, this, std::placeholders::_1)

	Application* Application::s_Instance = nullptr;

	Application::Application()
	: m_Camera()
	{
		// Important objects initialization
		ZE_CORE_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;

		m_Window = std::unique_ptr<Window>(Window::Create());
		m_Window->SetEventCallback(BIND_EVENT_FN(OnEvent));
		m_Context = GraphicsContext::Create(m_Window->GetNativeWindow());
		m_Context->Init();

		m_LayoutManager = LayoutManager::Create();
		Renderer::Init(m_LayoutManager);

		// Camera
		m_Camera = PerspectiveCamera(45.0f, (m_Window->GetWidth() / m_Window->GetHeight()), 0.1f, 10.0f);

		// Shader
		m_Shader = Shader::Create("Shader", "shader.spv");

		// Buffers and array config
		m_VertexArray = VertexArray::Create();
		float vertices[5 * 4] = {
			// Position   Color
			-0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
			 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
			 0.5f,  0.5f, 0.0f, 0.0f, 1.0f,
			-0.5f,  0.5f, 1.0f, 1.0f, 1.0f
		};

		uint32_t indices[6] = { 0, 1, 2, 2, 3, 0 };

		std::shared_ptr<VertexBuffer> vertexBuffer;
		vertexBuffer = VertexBuffer::Create(vertices, sizeof(vertices));

		BufferLayout layout = {
			{ ShaderDataType::Float2, "a_Position" },
			{ ShaderDataType::Float3, "a_Color" }
		};
		vertexBuffer->SetLayout(layout);

		std::shared_ptr<IndexBuffer> indexBuffer;
		indexBuffer = IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t));

		m_VertexArray->SetVertexBuffer(vertexBuffer);
		m_VertexArray->SetIndexBuffer(indexBuffer);

		// Pipeline state spec
		PipelineSpecification pipelineSpec { m_Shader, layout, false, false};
		m_PipelineState = PipelineState::Create(pipelineSpec, m_LayoutManager);

		m_FrameCommandBuffer = RenderCommandBuffer::Create();

		// ImGui layer
		m_ImGuiLayer = new ImGuiLayer();
		PushOverlay(m_ImGuiLayer);
	}

	Application::~Application() {
		if (m_Context)
			m_Context->WaitIdle();

		Renderer::Shutdown();
	}

	void Application::PushLayer(Layer* layer) {
		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverlay(Layer* layer) {
		m_LayerStack.PushOverlay(layer);
		layer->OnAttach();
	}

	void Application::OnEvent(Event& e) {
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(OnWindowResize));

		for (auto it = m_LayerStack.end(); it != m_LayerStack.begin(); ) {
			(*--it)->OnEvent(e);
			if (e.Handled)
				break;
		}
	}

	void Application::Run() {
		while (m_Running) {
			//
			for (Layer* layer : m_LayerStack)
				layer->OnUpdate();
			//

			uint32_t imageIndex = m_Context->AcquireNextImage();

			m_FrameCommandBuffer->Reset();
			m_FrameCommandBuffer->Begin();
			
			RenderCommand::BeginFrame(m_FrameCommandBuffer, imageIndex);
			RenderCommand::SetViewport(0, 0, m_Window->GetWidth(), m_Window->GetHeight());
			RenderCommand::SetClearColor(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));

			Renderer::BeginScene(m_Camera);
			Renderer::Submit(m_PipelineState, m_VertexArray);
			Renderer::EndScene();

			// ImGui overlay
			m_ImGuiLayer->Begin();
			for (Layer* layer : m_LayerStack) {
				layer->OnImGuiRender();
			}
			m_ImGuiLayer->End(m_FrameCommandBuffer);
			//

			RenderCommand::EndFrame();
			m_FrameCommandBuffer->End();

			m_Context->PresentImage(imageIndex, m_FrameCommandBuffer);

			m_Window->OnUpdate();
		}
	}

	bool Application::OnWindowClose(WindowCloseEvent& e) {
		m_Running = false;
		return true;
	}

	bool Application::OnWindowResize(WindowResizeEvent& e) {
		return true;
	}
}
