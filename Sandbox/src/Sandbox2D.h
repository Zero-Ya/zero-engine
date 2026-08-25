#pragma once

#include "ZEngine.h"

class Sandbox2D : public ZEngine::Layer{
public:
	Sandbox2D();
	virtual ~Sandbox2D() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	void OnUpdate(ZEngine::Timestep ts) override;
	virtual void OnImGuiRender() override;
	void OnEvent(ZEngine::Event& e) override;

private:

	ZEngine::OrthographicCameraController m_CameraController;

	ZEngine::Ref<ZEngine::VertexArray> m_SquareVertexArray;
	ZEngine::Ref<ZEngine::Shader> m_FlatShader;
	ZEngine::Ref<ZEngine::PipelineState> m_PipelineState;

	glm::vec4 m_SquareColor = { 0.0f, 0.0f, 0.0f, 0.0f };
};