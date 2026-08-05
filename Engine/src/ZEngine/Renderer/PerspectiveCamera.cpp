#include "PerspectiveCamera.h"

#include <glm/gtc/matrix_transform.hpp>

namespace ZEngine {

	PerspectiveCamera::PerspectiveCamera()
		: m_ModelMatrix(1.0f), m_ViewMatrix(1.0f), m_ProjectionMatrix(1.0f), m_ViewProjectionMatrix(1.0f)
	{}

	PerspectiveCamera::PerspectiveCamera(float fov, float aspect, float zNear, float zFar)
		: m_ModelMatrix(1.0f), m_ViewMatrix(1.0f), m_ProjectionMatrix(glm::perspective(glm::radians(fov), aspect, zNear, zFar))
	{
		m_ModelMatrix = rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		m_ViewMatrix = lookAt(glm::vec3(2.0f, 2.0f, 4.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

}