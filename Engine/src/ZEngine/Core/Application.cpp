#include "Application.h"
#include "Log.h"
#include "Input.h"

#include "ZEngine/Renderer/Renderer.h"

#include <GLFW/glfw3.h>

namespace ZEngine {

#define BIND_EVENT_FN(x) std::bind(&Application::x, this, std::placeholders::_1)

	Application* Application::s_Instance = nullptr;

	Application::Application() {
		// Important objects initialization
		ZE_CORE_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;

		m_Window = Scope<Window>(Window::Create());
		m_Window->SetEventCallback(BIND_EVENT_FN(OnEvent));
		m_Context = GraphicsContext::Create(m_Window->GetNativeWindow());
		m_Context->Init();

		ZEngine::Renderer::Init();
		m_FrameCommandBuffer = RenderCommandBuffer::Create();

		// ImGui layer
		m_ImGuiLayer = new ImGuiLayer();
		PushOverlay(m_ImGuiLayer);
	}

	Application::~Application() {
		if (m_Context)
			m_Context->WaitIdle();

		// Destroy sandbox before renderer
		m_LayerStack.~LayerStack();

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
			float time = (float)glfwGetTime();
			Timestep timestep = time - m_LastFrameTime;
			m_LastFrameTime = time;

			uint32_t imageIndex = m_Context->AcquireNextImage();

			m_FrameCommandBuffer->Reset();
			m_FrameCommandBuffer->Begin();

			ZEngine::RenderCommand::BeginFrame(m_FrameCommandBuffer, imageIndex);

			// Sandbox layers
			for (Layer* layer : m_LayerStack)
				layer->OnUpdate(timestep);

			// ImGui overlay
			m_ImGuiLayer->Begin();
			for (Layer* layer : m_LayerStack) {
				layer->OnImGuiRender();
			}
			m_ImGuiLayer->End(m_FrameCommandBuffer);
			//

			ZEngine::RenderCommand::EndFrame();
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
