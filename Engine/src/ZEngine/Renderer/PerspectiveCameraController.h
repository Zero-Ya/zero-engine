#pragma once

#include "ZEngine/Renderer/PerspectiveCamera.h"
#include "ZEngine/Core/Timestep.h"

#include "ZEngine/Events/ApplicationEvent.h"
#include "ZEngine/Events/MouseEvent.h"

namespace ZEngine {

	class PerspectiveCameraController {
	public:
		PerspectiveCameraController(float aspectRatio);

		void OnUpdate(Timestep ts);
		void OnEvent(Event& e);

		PerspectiveCamera& GetCamera() { return m_Camera; }
		const PerspectiveCamera& GetCamera() const { return m_Camera; }

	private:
		bool OnMouseScrolled(MouseScrolledEvent& e);
		bool OnWindowResized(WindowResizeEvent& e);

	private:
		float m_AspectRatio = 1.778f;
		float m_Zoom = 45.0f;

		glm::vec3 m_CameraPosition = { 0.0f, 0.0f, 0.0f };
		float m_Yaw = -90.0f;
		float m_Pitch = 0.0f;

		// Camera options
		float m_MovementSpeed = 5.0f;
		float m_MouseSensitivity = 0.0f;

		glm::vec2 m_LastMousePosition = { 0.0f, 0.0f };
		bool m_FirstMouse = true;

		PerspectiveCamera m_Camera;
	};

}