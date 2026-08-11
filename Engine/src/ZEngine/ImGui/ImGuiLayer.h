#pragma once

#include "ZEngine/Core/Layer.h"

#include "ZEngine/Events/ApplicationEvent.h"
#include "ZEngine/Events/KeyEvent.h"
#include "ZEngine/Events/MouseEvent.h"

namespace ZEngine {
	class VulkanImGuiUtil;
	class RenderCommandBuffer;

	class ImGuiLayer : public Layer {
	public:
		ImGuiLayer();
		~ImGuiLayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnImGuiRender() override;

		void Begin();
		void End(const Ref<RenderCommandBuffer>& renderCommandBuffer);

	private:
		float m_Time = 0.0f;
		bool m_ShowDemo = true;

	private:
		// Should be platform agnostic...
		Scope<VulkanImGuiUtil> m_Backend;
	};

}