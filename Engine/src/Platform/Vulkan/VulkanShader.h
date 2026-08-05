#pragma once

#include "ZEngine/Renderer/Shader.h"

#include <vulkan/vulkan_raii.hpp>

namespace ZEngine {

	class VulkanShader : public Shader {
	public:
		VulkanShader(std::string name,
					 const std::string& spirvFilePath,
					 std::string vertEntryPoint = "vertMain",
					 std::string fragEntryPoint = "fragMain");

		~VulkanShader() override = default;

		void Bind() const override {}
		void Unbind() const override {}

		const std::string& GetName() const override { return m_ShaderName; }

		const std::vector<vk::PipelineShaderStageCreateInfo>& GetShaderStages() const { return m_StageCreateInfos; }

	private:
		static std::vector<char> ReadSpirvFile(const std::string& filepath);
		void CreateShaderModuleAndStages();

	private:
		std::string m_ShaderName;
		std::string m_VertEntryPoint;
		std::string m_FragEntryPoint;
		
		std::vector<char> m_SpirvCode;

		vk::raii::ShaderModule m_ShaderModule = nullptr;
		std::vector<vk::PipelineShaderStageCreateInfo> m_StageCreateInfos;
	};

}