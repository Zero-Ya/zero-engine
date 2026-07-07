#include "Application.h"
#include "Log.h"

#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanSwapchain.h"
#include "Platform/Vulkan/VulkanPipelineManager.h"
#include "Platform/Vulkan/VulkanCommandManager.h"
#include "Platform/Vulkan/VulkanSyncManager.h"
#include "ZEngine/Renderer/RenderFrame.h"
#include "ZEngine/Renderer/ResourceManager.h"
#include "ZEngine/ImGui/ImGuiVulkanUtil.h"

#include "Input.h"

namespace ZEngine {

#define BIND_EVENT_FN(x) std::bind(&Application::x, this, std::placeholders::_1)

	Application* Application::s_Instance = nullptr;

	Application::Application() {
		ZE_CORE_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;

		m_Window = std::unique_ptr<Window>(Window::Create());
		m_Window->SetEventCallback(BIND_EVENT_FN(OnEvent));

		//m_ImGuiLayer = new ImGuiLayer();
		//PushOverlay(m_ImGuiLayer);

		// Vulkan resources initialization
		auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		vk_Ctx = std::make_unique<VulkanContext>(window);
		vk_Swapchain = std::make_unique<VulkanSwapchain>(vk_Ctx.get(), window);
		vk_CommandManager = std::make_unique<VulkanCommandManager>(vk_Ctx.get());
		resourceManager = std::make_unique<ResourceManager>(vk_Ctx.get(), vk_CommandManager.get(), vk_Swapchain.get());
		vk_Swapchain->SetResourceManager(resourceManager.get());
		vk_PipelineManager = std::make_unique<VulkanPipelineManager>(vk_Ctx.get(), vk_Swapchain.get(), resourceManager.get());
		vk_SyncManager = std::make_unique< VulkanSyncManager>(vk_Ctx.get(), vk_Swapchain.get());
		imguiUtil = std::make_unique<ImGuiVulkanUtil>(vk_Ctx.get(), vk_Swapchain.get(), vk_CommandManager.get(), resourceManager.get(), vk_SyncManager.get());
		frameRenderer = std::make_unique< RenderFrame>(
			vk_Ctx.get(),
			vk_Swapchain.get(),
			vk_SyncManager.get(),
			vk_PipelineManager.get(),
			vk_CommandManager.get(),
			resourceManager.get()
		);

		vk_Ctx->Init();
		vk_Swapchain->init();

		vk_PipelineManager->createDescriptorSetLayout();
		vk_PipelineManager->createGraphicsPipeline();
		vk_CommandManager->createCommandPool();

		// Textures and buffers //
		resourceManager->init();
		// //

		vk_PipelineManager->createDescriptorPool();
		vk_PipelineManager->createDescriptorSets();
		vk_CommandManager->createCommandBuffers();

		vk_SyncManager->init();
	}

	Application::~Application() {
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
			for (Layer* layer : m_LayerStack)
				layer->OnUpdate();

			//
			currentImageIndex = frameRenderer->beginFrame();
			frameRenderer->middleRecord(currentImageIndex);

			for (Layer* layer : m_LayerStack) {
				layer->OnRender();
			}

			frameRenderer->endFrame(currentImageIndex);
			//

			m_Window->OnUpdate();
		}

		vk_Ctx->getDevice().waitIdle();
	}

	bool Application::OnWindowClose(WindowCloseEvent& e) {
		m_Running = false;
		return true;
	}

	bool Application::OnWindowResize(WindowResizeEvent& e) {
		frameRenderer->framebufferResize();
		return true;
	}
}
