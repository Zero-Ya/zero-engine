#pragma once

#include "ZEngine/Asset/Model.h"

namespace ZEngine {

	class GraphicsDevice;
	class ResourceManager;
	class BindlessDescriptorManager;

	class ModelLoader {
	public:
		~ModelLoader() = default;

		virtual std::optional<Model> LoadModel(const std::string& filePath) = 0;

		static Scope<ModelLoader> Create(
			const Scope<GraphicsDevice>& graphicsDevice,
			const Scope<ResourceManager>& resourceManager,
			const Scope<BindlessDescriptorManager>& bindlessManager
		);
	};

}