#include "Texture.h"

#include "Renderer.h"
#include "Platform/Vulkan/VulkanTexture.h"

namespace ZEngine {

	Ref<Texture2D> Texture2D::Create(const std::string& path) {
		switch (Renderer::GetAPI()) {
			case RendererAPI::API::None:    ZE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::Vulkan:  return std::make_shared<VulkanTexture2D>(path);
		}

		ZE_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}