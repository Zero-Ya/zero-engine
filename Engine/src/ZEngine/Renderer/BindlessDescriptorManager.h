#pragma once

#include "ZEngine/RHI/RHICommon.h"

namespace ZEngine {

	class GraphicsDevice;
	class ResourceManager;

	constexpr uint32_t MAX_BINDLESS_TEXTURES   = 1024;
	constexpr uint32_t MAX_BINDLESS_SAMPLERS   = 16;
	constexpr uint32_t MAX_BINDLESS_STORAGES   = 10;
	constexpr uint32_t MAX_COMPARISON_SAMPLERS = 16;
	constexpr uint32_t MAX_BINDLESS_UNIFORMS   = 10;

	class BindlessDescriptorManager {
	public:
		virtual ~BindlessDescriptorManager() = default;

		virtual void SetResourceManager(const Scope<ResourceManager>& resourceManager) = 0;

		static Scope<BindlessDescriptorManager> Create(const Scope<GraphicsDevice>& graphicsDevice);

	};

}