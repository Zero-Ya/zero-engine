#pragma once

#include "ZEngine/Core/Layer.h"

#include "ZEngine/Events/ApplicationEvent.h"
#include "ZEngine/Events/KeyEvent.h"
#include "ZEngine/Events/MouseEvent.h"

namespace ZEngine {
	class Application;
	class VulkanSwapchain;
	class RenderFrame;
	class VulkanCommandManager;
	class ImGuiVulkanUtil;

	class ZE_API ImGuiLayer : public Layer {
	public:
		ImGuiLayer();
		~ImGuiLayer();

		void OnAttach();
		void OnDetach();
		void OnUpdate();
		void OnRender();
		void OnEvent(Event& event);

	private:
		float m_Time = 0.0f;

		bool OnMouseButtonPressedEvent(MouseButtonPressedEvent& e);
		bool OnMouseButtonReleasedEvent(MouseButtonReleasedEvent& e);
		bool OnMouseMovedEvent(MouseMovedEvent& e);
		bool OnMouseScrolledEvent(MouseScrolledEvent& e);
		bool OnKeyPressedEvent(KeyPressedEvent& e);
		bool OnKeyReleasedEvent(KeyReleasedEvent& e);
		bool OnKeyTypedEvent(KeyTypedEvent& e);
		//bool OnWindowResizeEvent(WindowResizeEvent& e);

		VulkanSwapchain* vk_Swapchain;
		RenderFrame* frameRenderer;
		VulkanCommandManager* vk_CommandManager;
		ImGuiVulkanUtil* imguiUtil;
	};

}