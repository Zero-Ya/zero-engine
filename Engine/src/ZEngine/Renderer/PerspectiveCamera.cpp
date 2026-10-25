#include "PerspectiveCamera.h"

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

	void PerspectiveCamera::NormalizePlane(Plane& plane) {
		float length = glm::length(plane.normal);
		if (length > 0.0f) {
			plane.normal /= length;
			plane.distance /= length;
		}
	}

	Frustum PerspectiveCamera::ExtractFrustum(const glm::mat4& viewProj) {
		Frustum frustum;

		auto getRow = [&](int row) {
			return glm::vec4(
				viewProj[0][row],
				viewProj[1][row],
				viewProj[2][row],
				viewProj[3][row]
			);
		};

		glm::vec4 r0 = getRow(0);
		glm::vec4 r1 = getRow(1);
		glm::vec4 r2 = getRow(2);
		glm::vec4 r3 = getRow(3);

		// Left plane (W + X)
		frustum.leftFace = { glm::vec3(r3 + r0), r3.w + r0.w };
		// Right plane (W - X)
		frustum.rightFace = { glm::vec3(r3 - r0), r3.w - r0.w };
		// Bottom plane (W + Y)
		frustum.bottomFace = { glm::vec3(r3 + r1), r3.w + r1.w };
		// Top plane (W - Y)
		frustum.topFace = { glm::vec3(r3 - r1), r3.w - r1.w };
		// Near plane (Z >= 0 in Vulkan [0, 1] depth range)
		frustum.nearFace = { glm::vec3(r2), r2.w };
		// Far lane (W - Z)
		frustum.farFace = { glm::vec3(r3 - r2), r3.w - r2.w };

		// Normalize all planes
		NormalizePlane(frustum.leftFace);
		NormalizePlane(frustum.rightFace);
		NormalizePlane(frustum.bottomFace);
		NormalizePlane(frustum.topFace);
		NormalizePlane(frustum.nearFace);
		NormalizePlane(frustum.farFace);

		return frustum;
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