#pragma once

#include "Core.h"

#include "Window.h"
#include "LayerStack.h"
#include "Events/Event.h"
#include "Events/ApplicationEvent.h"

namespace ZEngine {
	class VulkanContext;
	class VulkanSwapchain;
	class VulkanPipelineManager;
	class VulkanCommandManager;
	class VulkanSyncManager;
	class RenderFrame;
	class ResourceManager;

	class ZE_API Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();

		void OnEvent(Event& e);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

		inline Window& GetWindow() { return *m_Window; }

		inline static Application& Get() { return *s_Instance; }

	private:
		bool OnWindowClose(WindowCloseEvent& e);

		std::unique_ptr<Window> m_Window;
		bool m_Running = true;
		LayerStack m_LayerStack;

		static Application* s_Instance;

		std::unique_ptr<VulkanContext> vk_Ctx;
		std::unique_ptr<VulkanSwapchain> vk_Swapchain;
		std::unique_ptr<VulkanPipelineManager> vk_PipelineManager;
		std::unique_ptr<VulkanCommandManager> vk_CommandManager;
		std::unique_ptr<VulkanSyncManager> vk_SyncManager;
		std::unique_ptr<RenderFrame> frameRenderer;
		std::unique_ptr<ResourceManager> resourceManager;
	};

	// To be defined in CLIENT
	Application* CreateApplication();

}