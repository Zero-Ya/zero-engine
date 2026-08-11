#pragma once

namespace ZEngine {
	class RenderCommandBuffer;

	class GraphicsContext {
	public:
		virtual ~GraphicsContext() = default;

		virtual void Init() = 0;
		virtual void SwapBuffers() = 0;

		virtual uint32_t AcquireNextImage() = 0;
		virtual void PresentImage(uint32_t imageIndex, const Ref<RenderCommandBuffer>& renderCommandBuffer) = 0;
		virtual void RecreateSwapchain() = 0;
		virtual void WaitIdle() = 0;

		static Scope<GraphicsContext> Create(void* window);
	};

}