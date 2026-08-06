#pragma once

#include "Core.h"

#include "Window.h"
#include "LayerStack.h"
#include "ZEngine/Events/Event.h"
#include "ZEngine/Events/ApplicationEvent.h"

#include "ZEngine/Core/Timestep.h"

#include "ZEngine/ImGui/ImGuiLayer.h"

#include "ZEngine/Renderer/GraphicsContext.h"

namespace ZEngine {

	class Application {
	public:
		Application();
		virtual ~Application();

		void Run();

		void OnEvent(Event& e);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

		inline static Application& Get() { return *s_Instance; }
		inline Window& GetWindow() { return *m_Window; }
		inline GraphicsContext* GetGraphicsContext() { return m_Context.get(); }

	private:
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);

		std::unique_ptr<Window> m_Window;
		std::unique_ptr<GraphicsContext> m_Context;

		bool m_Running = true;

		ImGuiLayer* m_ImGuiLayer;
		LayerStack m_LayerStack;

		std::shared_ptr<RenderCommandBuffer> m_FrameCommandBuffer;

		float m_LastFrameTime = 0.0f;

	private:
		static Application* s_Instance;
	};

	// To be defined in CLIENT
	Application* CreateApplication();

}