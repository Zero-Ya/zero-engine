#include "VulkanPipelineManager.h"

namespace ZEngine {

	static std::vector<char> readFile(const std::string& filename);

	VulkanPipelineManager::VulkanPipelineManager(VulkanContext* ctx, VulkanSwapchain* swapchain, ResourceManager* resource) 
		: vk_Ctx(ctx), vk_Swapchain(swapchain), resourceManager(resource)
	{
	}

	VulkanPipelineManager::~VulkanPipelineManager() {}

	void VulkanPipelineManager::createDescriptorSetLayout()
	{
		std::array bindings = {
	vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex, nullptr),
	vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment, nullptr) };

		vk::DescriptorSetLayoutCreateInfo layoutInfo{ .bindingCount = static_cast<uint32_t>(bindings.size()), .pBindings = bindings.data() };
		descriptorSetLayout = vk::raii::DescriptorSetLayout(vk_Ctx->getDevice(), layoutInfo);
	}

	void VulkanPipelineManager::createGraphicsPipeline()
	{
		vk::raii::ShaderModule shaderModule = createShaderModule(readFile(ASSETS_DIR"/shaders/shader.spv"));


		vk::PipelineShaderStageCreateInfo vertShaderStageInfo{ .stage = vk::ShaderStageFlagBits::eVertex, .module = shaderModule, .pName = "vertMain" };
		vk::PipelineShaderStageCreateInfo fragShaderStageInfo{ .stage = vk::ShaderStageFlagBits::eFragment, .module = shaderModule, .pName = "fragMain" };
		vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		auto                                     bindingDescription = Vertex::getBindingDescription();
		auto                                     attributeDescriptions = Vertex::getAttributeDescriptions();
		vk::PipelineVertexInputStateCreateInfo   vertexInputInfo{ .vertexBindingDescriptionCount = 1,
																 .pVertexBindingDescriptions = &bindingDescription,
																 .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
																 .pVertexAttributeDescriptions = attributeDescriptions.data() };

		vk::PipelineInputAssemblyStateCreateInfo inputAssembly{ .topology = vk::PrimitiveTopology::eTriangleList };
		vk::PipelineViewportStateCreateInfo      viewportState{ .viewportCount = 1, .scissorCount = 1 };

		vk::PipelineRasterizationStateCreateInfo rasterizer{ .depthClampEnable = vk::False,
															.rasterizerDiscardEnable = vk::False,
															.polygonMode = vk::PolygonMode::eFill,
															.cullMode = vk::CullModeFlagBits::eBack,
															.frontFace = vk::FrontFace::eCounterClockwise,
															.depthBiasEnable = vk::False,
															.lineWidth = 1.0f };

		vk::PipelineMultisampleStateCreateInfo multisampling{ .rasterizationSamples = vk::SampleCountFlagBits::e1, .sampleShadingEnable = vk::False };

		vk::PipelineDepthStencilStateCreateInfo depthStencil{
		    .depthTestEnable       = vk::True,
		    .depthWriteEnable      = vk::True,
		    .depthCompareOp        = vk::CompareOp::eLess,
		    .depthBoundsTestEnable = vk::False,
		    .stencilTestEnable     = vk::False};

		vk::PipelineColorBlendAttachmentState colorBlendAttachment{
			.blendEnable = vk::False,
			.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA };

		vk::PipelineColorBlendStateCreateInfo colorBlending{
			.logicOpEnable = vk::False, .logicOp = vk::LogicOp::eCopy, .attachmentCount = 1, .pAttachments = &colorBlendAttachment };

		std::vector<vk::DynamicState>      dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
		vk::PipelineDynamicStateCreateInfo dynamicState{ .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()), .pDynamicStates = dynamicStates.data() };

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{ .setLayoutCount = 1, .pSetLayouts = &*descriptorSetLayout, .pushConstantRangeCount = 0 };
		pipelineLayout = vk::raii::PipelineLayout(vk_Ctx->getDevice(), pipelineLayoutInfo);

		vk::Format depthFormat = resourceManager->findDepthFormat();

		vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain = {
			{.stageCount = 2,
			 .pStages = shaderStages,
			 .pVertexInputState = &vertexInputInfo,
			 .pInputAssemblyState = &inputAssembly,
			 .pViewportState = &viewportState,
			 .pRasterizationState = &rasterizer,
			 .pMultisampleState = &multisampling,
			 .pDepthStencilState = &depthStencil,
			 .pColorBlendState = &colorBlending,
			 .pDynamicState = &dynamicState,
			 .layout = pipelineLayout,
			 .renderPass = nullptr},
			{.colorAttachmentCount = 1, .pColorAttachmentFormats = &vk_Swapchain->getFormat().format, .depthAttachmentFormat = depthFormat}};

		graphicsPipeline = vk::raii::Pipeline(vk_Ctx->getDevice(), nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
	}

	void VulkanPipelineManager::createDescriptorPool()
	{
		std::array poolSize{
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, MAX_FRAMES_IN_FLIGHT),
			vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, MAX_FRAMES_IN_FLIGHT) };
		vk::DescriptorPoolCreateInfo poolInfo{
			.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
			.maxSets = MAX_FRAMES_IN_FLIGHT,
			.poolSizeCount = static_cast<uint32_t>(poolSize.size()),
			.pPoolSizes = poolSize.data() };
		descriptorPool = vk::raii::DescriptorPool(vk_Ctx->getDevice(), poolInfo);
	}

	void VulkanPipelineManager::createDescriptorSets()
	{
		std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
		vk::DescriptorSetAllocateInfo        allocInfo{
				   .descriptorPool = descriptorPool,
				   .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
				   .pSetLayouts = layouts.data() };

		descriptorSets.clear();
		descriptorSets = vk_Ctx->getDevice().allocateDescriptorSets(allocInfo);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vk::DescriptorBufferInfo bufferInfo{
				.buffer = resourceManager->getUniformBuffers()[i],
				.offset = 0,
				.range = sizeof(UniformBufferObject) };
			vk::DescriptorImageInfo imageInfo{
				.sampler = resourceManager->getTextureSampler(),
				.imageView = resourceManager->getTextureImageView(),
				.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal };
			std::array descriptorWrites{
				vk::WriteDescriptorSet{
					.dstSet = descriptorSets[i],
					.dstBinding = 0,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = vk::DescriptorType::eUniformBuffer,
					.pBufferInfo = &bufferInfo},
				vk::WriteDescriptorSet{
					.dstSet = descriptorSets[i],
					.dstBinding = 1,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = vk::DescriptorType::eCombinedImageSampler,
					.pImageInfo = &imageInfo} };
			vk_Ctx->getDevice().updateDescriptorSets(descriptorWrites, {});
		}
	}

	[[nodiscard]] vk::raii::ShaderModule VulkanPipelineManager::createShaderModule(const std::vector<char>& code) const
	{
		vk::ShaderModuleCreateInfo createInfo{ .codeSize = code.size() * sizeof(char), .pCode = reinterpret_cast<const uint32_t*>(code.data()) };
		vk::raii::ShaderModule     shaderModule{ vk_Ctx->getDevice(), createInfo };

		return shaderModule;
	}

	static std::vector<char> readFile(const std::string& filename)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
		if (!file.is_open())
		{
			throw std::runtime_error("failed to open file!");
		}
		std::vector<char> buffer(file.tellg());
		file.seekg(0, std::ios::beg);
		file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
		file.close();
		return buffer;
	}
}