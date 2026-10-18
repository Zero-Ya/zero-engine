#pragma once

#include "Core.h"

#include "Window.h"
#include "LayerStack.h"
#include "ZEngine/Events/Event.h"
#include "ZEngine/Events/ApplicationEvent.h"

#include "ZEngine/Core/Timestep.h"

#include "ZEngine/ImGui/ImGuiLayer.h"

#include "ZEngine/RHI/GraphicsDevice.h"
#include "ZEngine/Renderer/ResourceManager.h"
#include "ZEngine/Renderer/BindlessDescriptorManager.h"

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
		inline Scope<GraphicsDevice>& GetGraphicsDevice() { return m_GraphicsDevice; }
		inline ImGuiLayer* GetImGuiLayer() { return m_ImGuiLayer; }

		inline Scope<ResourceManager>& GetResourceManager() { return m_ResourceManager; }
		inline Scope<BindlessDescriptorManager>& GetBindlessManager() { return m_BindlessManager; }

	private:
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);

		Scope<Window> m_Window;
		Scope<GraphicsDevice> m_GraphicsDevice;

		Scope<ResourceManager> m_ResourceManager;
		Scope<BindlessDescriptorManager> m_BindlessManager;

		ImGuiLayer* m_ImGuiLayer;
		LayerStack m_LayerStack;

		bool m_Running = true;
		float m_LastFrameTime = 0.0f;

	private:
		static Application* s_Instance;
	};

	// To be defined in CLIENT
	Application* CreateApplication();

}