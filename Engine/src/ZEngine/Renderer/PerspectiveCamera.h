#pragma once

#include <glm/glm.hpp>

namespace ZEngine {

	class PerspectiveCamera {
	public:
		PerspectiveCamera();
		PerspectiveCamera(float fov, float aspect, float zNear, float zFar);

		const glm::mat4& GetModelMatrix() const { return m_ModelMatrix; }
		const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
		const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
		const glm::mat4& GetViewProjectionMatrix() const { return m_ViewProjectionMatrix; }

	private:
		glm::mat4 m_ModelMatrix;
		glm::mat4 m_ViewMatrix;
		glm::mat4 m_ProjectionMatrix;
		glm::mat4 m_ViewProjectionMatrix;
	};

}