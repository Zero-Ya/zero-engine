#include "Shader.h"

#include "Renderer.h"
#include "Platform/Vulkan/VulkanShader.h"

namespace ZEngine {

	Ref<Shader> Shader::Create(const std::string& spirvFilePath) {
		switch (Renderer::GetAPI()) {
			case RendererAPI::API::None:
				ZE_CORE_ASSERT(false, "RendererAPI::None is currently unsupported!");
				return nullptr;
			case RendererAPI::API::Vulkan:
				return std::make_shared<VulkanShader>(spirvFilePath);
		}
		ZE_CORE_ASSERT(false, "Unknown RendererAPI backend encountered during VulkanShader allocation!");
		return nullptr;
	}

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

	void ShaderLibrary::Add(const std::string& name, const Ref<Shader>& shader) {
		ZE_CORE_ASSERT(!Exists(name), "Shader already exists!");
		m_Shaders[name] = shader;
	}

	void ShaderLibrary::Add(const Ref<Shader>& shader) {
		auto& name = shader->GetName();
		Add(name, shader);
	}

	Ref<Shader> ShaderLibrary::Load(const std::string& filepath) {
		auto shader = Shader::Create(filepath);
		Add(shader);
		return shader;
	}

	Ref<Shader> ShaderLibrary::Load(const std::string& name, const std::string& filepath) {
		auto shader = Shader::Create(filepath);
		Add(name, shader);
		return shader;
	}

	Ref<Shader> ShaderLibrary::Get(const std::string& name) {
		ZE_CORE_ASSERT(Exists(name), "Shader not found!");
		return m_Shaders[name];
	}

	bool ShaderLibrary::Exists(const std::string& name) const {
		return m_Shaders.find(name) != m_Shaders.end();
	}

}