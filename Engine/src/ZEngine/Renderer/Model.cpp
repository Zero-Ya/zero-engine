#include "Model.h"

#include "Renderer.h"
#include "Platform/Vulkan/VulkanModel.h"

namespace ZEngine {

	Scope<Model> Model::Create() {
		switch (Renderer::GetAPI()) {
			case RendererAPI::API::None:    ZE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::Vulkan:  return std::make_unique<VulkanModel>();
		}

		ZE_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}