#pragma once

#include "ZEngine/Renderer/PipelineState.h"

#include <vulkan/vulkan_raii.hpp>

namespace ZEngine {

	class VulkanPipelineState : public PipelineState {
	public:
		VulkanPipelineState(const PipelineSpecification& spec, const Scope<LayoutManager>& layoutManager);
		virtual ~VulkanPipelineState() override = default;

		virtual void Bind() const override {}

		const vk::raii::Pipeline& GetNativePipeline() const { return m_Pipeline; }
		const vk::raii::PipelineLayout& GetNativeLayout() const { return m_PipelineLayout; }

	private:
		vk::Format ShaderDataTypeToVulkanFormat(ShaderDataType type);
		void CompileGraphicsPipeline(const PipelineSpecification& spec);
		[[nodiscard]] vk::raii::ShaderModule CreateShaderModule(const std::vector<char>& code);

	private:
		vk::raii::PipelineLayout m_PipelineLayout = nullptr;
		vk::raii::Pipeline		 m_Pipeline = nullptr;
	};

}