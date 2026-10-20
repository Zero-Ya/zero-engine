#pragma once

#include "ZEngine/Renderer/PipelineState.h"

#include "ZEngine/Asset/ShaderTypes.h"

namespace ZEngine {

	class GraphicsDevice;
	class BindlessDescriptorManager;
	class ResourceManager;

	class Cubemap {
	public:
		virtual ~Cubemap() = default;

		virtual void Init(const PipelineSpecification& spec) = 0;
		virtual std::optional<CubemapData> LoadCubemap(std::vector<std::string> faces) = 0;

		static Scope<Cubemap> Create(const Scope<GraphicsDevice>& graphicsDevice, const Scope<BindlessDescriptorManager>& bindlessManager, const Scope<ResourceManager>& resourceManager);
	};

}