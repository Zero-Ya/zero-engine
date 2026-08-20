#include "PerspectiveCamera.h"

#include <glm/gtc/matrix_transform.hpp>

namespace ZEngine {

	PerspectiveCamera::PerspectiveCamera(float fov, float aspectRatio, float zNear, float zFar)
		: m_Zoom(fov), m_AspectRatio(aspectRatio), m_Near(zNear), m_Far(zFar), m_ModelMatrix(1.0f)
	{
		RecalculateProjection();
		UpdateCameraVectors();
	}

	void PerspectiveCamera::SetProjection(float fov, float aspectRatio, float zNear, float zFar) {
		m_Zoom = fov;
		m_AspectRatio = aspectRatio;
		m_Near = zNear;
		m_Far = zFar;
		RecalculateProjection();
	}

	void PerspectiveCamera::SetPosition(glm::vec3 pos) {
		m_Position = pos;
		RecalculateView();
	}

	void PerspectiveCamera::RecalculateProjection() {
		m_ProjectionMatrix = glm::perspective(glm::radians(m_Zoom), m_AspectRatio, m_Near, m_Far);
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

	void PerspectiveCamera::RecalculateView() {
		m_ViewMatrix = glm::lookAt(m_Position, m_Position + m_Front, m_Up);
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

	void PerspectiveCamera::UpdateCameraVectors() {
		// Calculate the new Front vector
		glm::vec3 front{};
		front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
		front.y = sin(glm::radians(m_Pitch));
		front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
		m_Front = glm::normalize(front);
		// Also re-calculate the Right and Up vector
													 // m_WorldUp
		m_Right = glm::normalize(glm::cross(m_Front, glm::vec3(0.0f, 1.0f, 0.0f)));
		m_Up = glm::normalize(glm::cross(m_Right, m_Front));

		RecalculateView();
	}

}