#pragma once

#include "ZEngine/RHI/RHICommon.h"

namespace ZEngine {

	class GraphicsDevice;

	class ResourceManager {
	public:
		virtual ~ResourceManager() = default;

		virtual void ImportExternalResources() = 0;

		virtual BufferHandle CreateBuffer(const BufferCreateInfo& info) = 0;
		virtual void DestroyBuffer(BufferHandle handle) = 0;

		virtual TextureHandle CreateTexture(const TextureCreateInfo& info) = 0;
		virtual void DestroyTexture(TextureHandle handle) = 0;

		static Scope<ResourceManager> Create(const Scope<GraphicsDevice>& graphicsDevice);
	};

}