#pragma once

#include "Core.h"

#include "Window.h"
#include "LayerStack.h"
#include "ZEngine/Events/Event.h"
#include "ZEngine/Events/ApplicationEvent.h"

#include "ZEngine/ImGui/ImGuiLayer.h"

#include "ZEngine/Renderer/GraphicsContext.h"
#include "ZEngine/Renderer/LayoutManager.h"
#include "ZEngine/Renderer/Shader.h"
#include "ZEngine/Renderer/VertexArray.h"
#include "ZEngine/Renderer/PipelineState.h"

#include "ZEngine/Renderer/PerspectiveCamera.h"

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

		ImGuiLayer* m_ImGuiLayer;
		LayerStack m_LayerStack;

		bool m_Running = true;
		
		std::unique_ptr<LayoutManager> m_LayoutManager;
		std::shared_ptr<Shader> m_Shader;
		std::shared_ptr<RenderCommandBuffer> m_FrameCommandBuffer;
		std::shared_ptr<VertexArray> m_VertexArray;
		std::shared_ptr<PipelineState> m_PipelineState;

		PerspectiveCamera m_Camera;

	private:
		static Application* s_Instance;
	};

	// To be defined in CLIENT
	Application* CreateApplication();

}