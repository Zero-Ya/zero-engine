#include "RendererAPI.h"
#include "Platform/Vulkan/VulkanRendererAPI.h"

namespace ZEngine {

	RendererAPI::API RendererAPI::s_API = RendererAPI::API::Vulkan;

	std::unique_ptr<RendererAPI> RendererAPI::Create() {
		switch (s_API) {
			case RendererAPI::API::None:    ZE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::Vulkan:  return std::make_unique<VulkanRendererAPI>();
		}
		ZE_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}