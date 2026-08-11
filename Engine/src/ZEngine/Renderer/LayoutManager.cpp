#include "LayoutManager.h"

#include "Renderer.h"
#include "Platform/Vulkan/VulkanLayoutManager.h"

namespace ZEngine {

	Scope<LayoutManager> LayoutManager::Create() {
		switch (Renderer::GetAPI()) {
			case RendererAPI::API::None:
				ZE_CORE_ASSERT(false, "RendererAPI::None is currently unsupported!");
				return nullptr;
			case RendererAPI::API::Vulkan:
				return std::make_unique<VulkanLayoutManager>();
		}
		ZE_CORE_ASSERT(false, "Unknown RendererAPI backend encountered during VulkanLayoutManager allocation!");
		return nullptr;
	}

}