#include "PipelineState.h"

#include "Platform/Vulkan/VulkanPipelineState.h"

namespace ZEngine {

	Ref<PipelineState> PipelineState::Create(const PipelineSpecification& spec) {
		return std::make_shared<VulkanPipelineState>(spec);
	}

}