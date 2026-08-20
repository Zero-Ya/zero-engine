#pragma once

#include <glm/glm.hpp>

namespace ZEngine {

	class PerspectiveCamera {
	public:
		PerspectiveCamera(float fov, float aspectRatio, float zNear, float zFar);

		const glm::mat4& GetModelMatrix() const { return m_ModelMatrix; }
		const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
		const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
		const glm::mat4& GetViewProjectionMatrix() const { return m_ViewProjectionMatrix; }

		const glm::vec3& GetPosition() const { return m_Position; }
		const glm::vec3& GetFrontVector() const { return m_Front; }
		const glm::vec3& GetRightVector() const { return m_Right; }

		void SetProjection(float fov, float aspectRatio, float zNear, float zFar);
		void SetPosition(glm::vec3 pos);
		void SetRotation(float pitch, float yaw) { m_Pitch = pitch; m_Yaw = yaw; UpdateCameraVectors(); }

	private:
		void RecalculateProjection();
		void RecalculateView();
		void UpdateCameraVectors();

	private:
		glm::mat4 m_ModelMatrix;
		glm::mat4 m_ViewMatrix;
		glm::mat4 m_ProjectionMatrix;
		glm::mat4 m_ViewProjectionMatrix;

		// Euler angles
		float m_Yaw = -90.0f;
		float m_Pitch = 0.0f;

		// Camera attributes
		glm::vec3 m_Front = { 0.0f, 0.0f, -1.0f };
		glm::vec3 m_Up = { 0.0f, 1.0f, 0.0f };
		glm::vec3 m_Right = { 1.0f, 0.0f, 0.0f };
		//glm::vec3 m_WorldUp;

		float m_Zoom = 45.0f;
		float m_AspectRatio = 1.778f;
		float m_Near = 0.1f, m_Far = 100.0f;

		glm::vec3 m_Position = { 0.0f, 0.0f, 0.0f };
	};

}