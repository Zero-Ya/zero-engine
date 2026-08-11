#pragma once

namespace ZEngine {

	class RenderCommandBuffer {
	public:
		virtual ~RenderCommandBuffer() = default;

		virtual void Begin() = 0;
		virtual void End() = 0;
		virtual void Reset() = 0;

		static Ref<RenderCommandBuffer> Create();
	};

}