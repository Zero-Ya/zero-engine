#include "RenderCommandBuffer.h"
#include "Renderer.h"
#include "Platform/Vulkan/VulkanCommandBuffer.h"

namespace ZEngine {

	Ref<RenderCommandBuffer> RenderCommandBuffer::Create() {
		switch (Renderer::GetAPI()) {
			case RendererAPI::API::None: return nullptr;
			case RendererAPI::API::Vulkan: return std::make_shared<VulkanCommandBuffer>();
		}
		ZE_CORE_ASSERT(false, "Unknown RendererAPI backend encountered during RenderCommandBuffer allocation!");
		return nullptr;
	}

}