#include "PipelineState.h"
#include "Renderer.h"
#include "Platform/Vulkan/VulkanPipelineState.h"

namespace ZEngine {

	std::shared_ptr<PipelineState> PipelineState::Create(const PipelineSpecification& spec, const std::unique_ptr<LayoutManager>& layoutManager) {
		switch (Renderer::GetAPI()) {
			case RendererAPI::API::None: return nullptr;
			case RendererAPI::API::Vulkan: return std::make_shared<VulkanPipelineState>(spec, layoutManager);
		}
		return nullptr;
	}

}