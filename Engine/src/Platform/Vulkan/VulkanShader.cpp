#include "VulkanShader.h"
#include "VulkanContext.h"
#include "ZEngine/Core/Application.h"

namespace ZEngine {
	VulkanShader::VulkanShader(const std::string& spirvFilePath) {
		m_SpirvCode = ReadSpirvFile(spirvFilePath);
		CreateShaderModuleAndStages();
	}

	// Don't really know why we needed name...
	VulkanShader::VulkanShader(std::string name,
		const std::string& spirvFilePath,
		std::string vertEntryPoint,
		std::string fragEntryPoint)
		: m_ShaderName(name),
		  m_VertEntryPoint(vertEntryPoint),
		  m_FragEntryPoint(fragEntryPoint)
	{
		m_SpirvCode = ReadSpirvFile(spirvFilePath);
		CreateShaderModuleAndStages();
	}

	std::vector<char> VulkanShader::ReadSpirvFile(const std::string& filepath) {
		std::ifstream file(ASSETS_DIR"/shaders/" + filepath, std::ios::ate | std::ios::binary);
		if (!file.is_open())
		{
			throw std::runtime_error("VulkanShader failed to open SPIR-V file!");
		}
		std::vector<char> buffer(file.tellg());
		file.seekg(0, std::ios::beg);
		file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
		file.close();
		return buffer;
	}

	void VulkanShader::CreateShaderModuleAndStages() {
		auto vk_Context = static_cast<VulkanContext*>(ZEngine::Application::Get().GetGraphicsContext());
		auto& device = vk_Context->GetDevice();

		vk::ShaderModuleCreateInfo createInfo { .codeSize = m_SpirvCode.size() * sizeof(char), .pCode = reinterpret_cast<const uint32_t*>(m_SpirvCode.data()) };
		m_ShaderModule = vk::raii::ShaderModule(device, createInfo);

		vk::PipelineShaderStageCreateInfo vertShaderStageInfo{ .stage = vk::ShaderStageFlagBits::eVertex, .module = m_ShaderModule, .pName = "vertMain" };
		vk::PipelineShaderStageCreateInfo fragShaderStageInfo{ .stage = vk::ShaderStageFlagBits::eFragment, .module = m_ShaderModule, .pName = "fragMain" };

		m_StageCreateInfos.emplace_back(vertShaderStageInfo);
		m_StageCreateInfos.emplace_back(fragShaderStageInfo);
	}

}