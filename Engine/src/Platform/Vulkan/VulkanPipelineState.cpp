#include "VulkanPipelineState.h"
#include "VulkanContext.h"
#include "ZEngine/Core/Application.h"
#include "VulkanCommandBuffer.h"
#include "VulkanShader.h"
#include "VulkanLayoutManager.h"

namespace {

	std::vector<char> ReadFile(const std::string& filename);

}

namespace ZEngine {

	VulkanPipelineState::VulkanPipelineState(const PipelineSpecification& spec, const Scope<LayoutManager>& layoutManager) {
		auto vk_Context = static_cast<VulkanContext*>(Application::Get().GetGraphicsContext());
		auto vk_Shader = static_cast<VulkanShader*>(spec.Shader.get());
		auto& device = vk_Context->GetDevice();

		// Shader stages info
		std::vector<vk::PipelineShaderStageCreateInfo> shaderStages = vk_Shader->GetShaderStages();

		// Vertex input info
		uint32_t stride = 0;
		for (const auto& element : spec.Layout) {
			stride += element.Size;
		}
		vk::VertexInputBindingDescription bindingDescription( 0, stride, vk::VertexInputRate::eVertex);

		std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
		uint32_t location = 0;
		uint32_t offset = 0;
		for (const auto& element : spec.Layout) {
			attributeDescriptions.push_back({ location++, 0, ShaderDataTypeToVulkanFormat(element.Type), offset });
			offset += element.Size;
		}
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo { .vertexBindingDescriptionCount = 1,
																 .pVertexBindingDescriptions = &bindingDescription,
																 .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
																 .pVertexAttributeDescriptions = attributeDescriptions.data() };

		// Rasterizer info
		vk::PipelineRasterizationStateCreateInfo rasterizerInfo { .depthClampEnable = vk::False,
																  .rasterizerDiscardEnable = vk::False,
																  .polygonMode = vk::PolygonMode::eFill,
																  .cullMode = vk::CullModeFlagBits::eNone,
																  .frontFace = vk::FrontFace::eCounterClockwise,
																  .depthBiasEnable = vk::False,
																  .lineWidth = 1.0f };
		// Multisampling and color blending
		vk::PipelineMultisampleStateCreateInfo multisampling { .rasterizationSamples = vk::SampleCountFlagBits::e1, .sampleShadingEnable = vk::False };

		// Depth stencil
		vk::PipelineDepthStencilStateCreateInfo depthStencil{
			.depthTestEnable = vk::True,
			.depthWriteEnable = vk::True,
			.depthCompareOp = vk::CompareOp::eLess,
			.depthBoundsTestEnable = vk::False,
			.stencilTestEnable = vk::False
		};

		vk::PipelineColorBlendAttachmentState  colorBlendAttachment{
			.blendEnable = vk::True,
			// RGB blending
			.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
			.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
			.colorBlendOp = vk::BlendOp::eAdd,

			// Alpha blending
			.srcAlphaBlendFactor = vk::BlendFactor::eOne,
			.dstAlphaBlendFactor = vk::BlendFactor::eZero,
			.alphaBlendOp = vk::BlendOp::eAdd,

			.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
		};

		vk::PipelineColorBlendStateCreateInfo  colorBlending {
			.logicOpEnable = vk::False, .logicOp = vk::LogicOp::eCopy, .attachmentCount = 1, .pAttachments = &colorBlendAttachment };

		// Input assembly and viewport state
		vk::PipelineInputAssemblyStateCreateInfo inputAssembly { .topology = vk::PrimitiveTopology::eTriangleList };
		vk::PipelineViewportStateCreateInfo		 viewportState { .viewportCount = 1, .scissorCount = 1 };

		// Dynamic state info
		std::vector<vk::DynamicState>	   dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
		vk::PipelineDynamicStateCreateInfo dynamicState { .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()), .pDynamicStates = dynamicStates.data() };

		// Pipeline layout info
		auto g_LayoutManager = static_cast<VulkanLayoutManager*>(layoutManager.get());
		m_PipelineLayout = g_LayoutManager->GetGlobalPipelineLayout();

		vk::Format depthFormat = vk_Context->GetDepthFormat();
		vk::Format colorFormat = vk::Format::eB8G8R8A8Srgb; // We can also get swapchain surface format
		vk::PipelineRenderingCreateInfo dynamicRenderingInfo{ .colorAttachmentCount = 1, .pColorAttachmentFormats = &colorFormat, .depthAttachmentFormat = depthFormat };

		// Pipeline info
		vk::GraphicsPipelineCreateInfo pipelineInfo {
			.pNext = &dynamicRenderingInfo,
			.stageCount = 2,
			.pStages = shaderStages.data(),
			.pVertexInputState = &vertexInputInfo,
			.pInputAssemblyState = &inputAssembly,
			.pViewportState = &viewportState,
			.pRasterizationState = &rasterizerInfo,
			.pMultisampleState = &multisampling,
			.pDepthStencilState = &depthStencil,
			.pColorBlendState = &colorBlending,
			.pDynamicState = &dynamicState,
			.layout = m_PipelineLayout
		};

		m_Pipeline = vk::raii::Pipeline(device, nullptr, pipelineInfo);
	}

	vk::Format VulkanPipelineState::ShaderDataTypeToVulkanFormat(ShaderDataType type) {
		switch (type) {
			case ShaderDataType::Float:  return vk::Format::eR32Sfloat;
			case ShaderDataType::Float2: return vk::Format::eR32G32Sfloat;
			case ShaderDataType::Float3: return vk::Format::eR32G32B32Sfloat;
			case ShaderDataType::Float4: return vk::Format::eR32G32B32A32Sfloat;
			default: ZE_CORE_ASSERT(false, "Unknown ShaderDataType passed to Vulkan pipeline layout compiler!");
		}
		return vk::Format::eUndefined;
	}

	void VulkanPipelineState::CompileGraphicsPipeline(const PipelineSpecification& spec) {

	}

	[[nodiscard]] vk::raii::ShaderModule VulkanPipelineState::CreateShaderModule(const std::vector<char>& code) {
		auto vk_Context = static_cast<VulkanContext*>(ZEngine::Application::Get().GetGraphicsContext());
		auto& device = vk_Context->GetDevice();

		vk::ShaderModuleCreateInfo createInfo{ .codeSize = code.size() * sizeof(char), .pCode = reinterpret_cast<const uint32_t*>(code.data()) };
		vk::raii::ShaderModule shaderModule{ device, createInfo };

		return shaderModule;
	}

}

namespace {

	std::vector<char> ReadFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
		if (!file.is_open())
		{
			throw std::runtime_error("Failed to open file!");
		}
		std::vector<char> buffer(file.tellg());
		file.seekg(0, std::ios::beg);
		file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
		file.close();
		return buffer;
	}

}