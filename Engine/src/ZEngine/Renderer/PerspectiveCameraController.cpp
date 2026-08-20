#pragma once

#include "PerspectiveCameraController.h"

#include <glm/gtc/matrix_transform.hpp>

#include "ZEngine/Core/Input.h"
#include "ZEngine/Core/KeyCodes.h"
#include "ZEngine/Core/MouseButtonCodes.h"

#include "ZEngine/Core/Application.h"

namespace ZEngine {

    PerspectiveCameraController::PerspectiveCameraController(float aspectRatio)
        : m_AspectRatio(aspectRatio),
          m_Camera(45.0f, aspectRatio, 0.1f, 100.0f),
          m_MovementSpeed(5.0f),
          m_MouseSensitivity(0.1f),
          m_Zoom(45.0f)
    {}

    void PerspectiveCameraController::OnUpdate(Timestep ts) {
        // Mouse
        if (Input::IsMouseButtonPressed(ZE_MOUSE_BUTTON_RIGHT)) { // Or lock mouse cursor 
            glm::vec2 mousePos = glm::vec2(Input::GetMouseX(), Input::GetMouseY());

            if (m_FirstMouse) {
                m_LastMousePosition = mousePos;
                m_FirstMouse = false;
            }

            float xOffset = (mousePos.x - m_LastMousePosition.x) * m_MouseSensitivity;
            float yOffset = (m_LastMousePosition.y - mousePos.y) * m_MouseSensitivity; // Inverted Y

            m_LastMousePosition = mousePos;

            m_Yaw += xOffset;
            m_Pitch += yOffset;

            // Clamp Pitch to prevent camera flipping upside down (Gimbal Lock protection)
            m_Pitch = std::clamp(m_Pitch, -89.0f, 89.0f);

            m_Camera.SetRotation(m_Pitch, m_Yaw);

        } else {
            m_FirstMouse = true; // Reset initial offset when releasing right-click
        }

        // WASD movement
        float velocity = m_MovementSpeed * ts;

        if (Input::IsKeyPressed(ZE_KEY_W))
            m_CameraPosition += m_Camera.GetFrontVector() * velocity;
        if (Input::IsKeyPressed(ZE_KEY_S))
            m_CameraPosition -= m_Camera.GetFrontVector() * velocity;

        if (Input::IsKeyPressed(ZE_KEY_A))
            m_CameraPosition -= m_Camera.GetRightVector() * velocity;
        if (Input::IsKeyPressed(ZE_KEY_D))
            m_CameraPosition += m_Camera.GetRightVector() * velocity;

        // Vertical movement
        if (Input::IsKeyPressed(ZE_KEY_E))
            m_CameraPosition.y += velocity;
        if (Input::IsKeyPressed(ZE_KEY_Q))
            m_CameraPosition.y -= velocity;

        m_Camera.SetPosition(m_CameraPosition);
    }

    void PerspectiveCameraController::OnEvent(Event& e) {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<MouseScrolledEvent>(ZE_BIND_EVENT_FN(PerspectiveCameraController::OnMouseScrolled));
    }

    bool PerspectiveCameraController::OnMouseScrolled(MouseScrolledEvent& e) {
        m_Zoom -= e.GetYOffset() * 0.5f;
        m_Zoom = std::clamp(m_Zoom, 1.0f, 45.0f);
        m_Camera.SetProjection(m_Zoom, m_AspectRatio, 0.1f, 100.0f);
        return false;
    }

}