#include "PipelineState.h"
#include "Renderer.h"
#include "Platform/Vulkan/VulkanPipelineState.h"

namespace ZEngine {

	Ref<PipelineState> PipelineState::Create(const PipelineSpecification& spec) {
		switch (Renderer::GetAPI()) {
			case RendererAPI::API::None: return nullptr;
			case RendererAPI::API::Vulkan: return std::make_shared<VulkanPipelineState>(spec);
		}
		return nullptr;
	}

}