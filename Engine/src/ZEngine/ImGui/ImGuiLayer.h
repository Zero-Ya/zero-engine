#pragma once

#include "ZEngine/Core/Layer.h"

#include "ZEngine/Events/ApplicationEvent.h"
#include "ZEngine/Events/KeyEvent.h"
#include "ZEngine/Events/MouseEvent.h"

#include "ZEngine/Asset/ShaderTypes.h"

namespace ZEngine {
	class VulkanImGuiUtil;
	class RenderCommandBuffer;

	class ImGuiLayer : public Layer {
	public:
		ImGuiLayer();
		~ImGuiLayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		void OnImGuiRender(DirectionalLight& directionalLight);

		void Begin();
		void End();

	private:
		float m_Time = 0.0f;
		bool m_ShowDemo = true;

	private:
		// Should be platform agnostic...
		Scope<VulkanImGuiUtil> m_Backend;
	};

}