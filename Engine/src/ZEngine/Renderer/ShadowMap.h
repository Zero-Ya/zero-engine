#pragma once

#include "ZEngine/RHI/RHICommon.h"
#include "ZEngine/Asset/ShaderTypes.h"

#include "ZEngine/Renderer/Buffer.h"
#include "ZEngine/Renderer/PipelineState.h"

#include <glm/glm.hpp>

namespace ZEngine {

	class GraphicsDevice;
	class BindlessDescriptorManager;
	class ResourceManager;

	class ShadowMap {
	public:
		virtual ~ShadowMap() = default;

		virtual TextureHandle Init(const PipelineSpecification& spec, uint32_t shadowResolution = 2048) = 0;

		static Ref<ShadowMap> Create(const Scope<GraphicsDevice>& graphicsDevice, const Scope<BindlessDescriptorManager>& bindlessManager, const Scope<ResourceManager>& resourceManager);
	};

}