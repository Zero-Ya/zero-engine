#include "Shader.h"

#include "Renderer.h"
#include "Platform/Vulkan/VulkanShader.h"

namespace ZEngine {

	Ref<Shader> Shader::Create(const std::string& name,
										  const std::string& spirvFilePath,
										  const std::string& vertEntryPoint,
										  const std::string& fragEntryPoint)
	{
		switch (Renderer::GetAPI()) {
			case RendererAPI::API::None:
				ZE_CORE_ASSERT(false, "RendererAPI::None is currently unsupported!");
				return nullptr;
			case RendererAPI::API::Vulkan:
				return std::make_shared<VulkanShader>(name, spirvFilePath, vertEntryPoint, fragEntryPoint);
		}
		ZE_CORE_ASSERT(false, "Unknown RendererAPI backend encountered during VulkanShader allocation!");
		return nullptr;
	}

}