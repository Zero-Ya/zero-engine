#include "Material.h"

#include "Renderer.h"
#include "Platform/Vulkan/VulkanMaterial.h"

namespace ZEngine {

	Ref<Material> Material::Create(const std::string& name, const Ref<Texture2D>& texture) {
		switch (Renderer::GetAPI()) {
			case RendererAPI::API::None:    ZE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::Vulkan:  return std::make_shared<VulkanMaterial>(name, texture);
		}

		ZE_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}