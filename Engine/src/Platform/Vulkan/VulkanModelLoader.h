#pragma once

#include "ZEngine/Renderer/ModelLoader.h"

#include <vulkan/vulkan_raii.hpp>

#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace ZEngine {

	class VulkanGraphicsDevice;
	class VulkanResourceManager;
	class VulkanBindlessManager;

	class VulkanModelLoader : public ModelLoader {
	public:
		VulkanModelLoader(
			const Scope<GraphicsDevice>& graphicsDevice,
			const Scope<ResourceManager>& resourceManager,
			const Scope<BindlessDescriptorManager>& bindlessManager
		);
		~VulkanModelLoader() = default;

		std::optional<Model> LoadModel(const std::string& filePath) override;

	private:
		TextureHandle LoadGLTFTexture(const std::string& baseDir, const fastgltf::Image& image);
		uint32_t RegisterTexture(const fastgltf::Asset& asset, uint32_t textureIndex, std::vector<TextureHandle>& loadedImages);
		uint32_t RegisterSampler(const fastgltf::Sampler& sampler);

		void LoadSubmeshes(const fastgltf::Asset& asset, const fastgltf::Mesh& gltfMesh, Mesh& engineMesh, std::vector<uint32_t>& textureBindlessIndices, std::vector<uint32_t>& samplerBindlessIndices);
		void LoadSceneNodes(const fastgltf::Asset& asset, uint32_t nodeIndex, std::vector<std::shared_ptr<Node>> allNodes);
		void LoadMaterials(const fastgltf::Asset& asset);

		TextureHandle LoadTexture(const std::string& filePath);
		TextureHandle LoadStandardImage(const std::string& filePath);
		TextureHandle LoadKTXTexture(const std::string& filePath);

	private:
		void GetFilterModes(std::optional<fastgltf::Filter> filter, vk::Filter& outFilter, vk::SamplerMipmapMode& outMipMode);
		vk::SamplerAddressMode GetAddressMode(fastgltf::Wrap wrap);
		TextureFormat GetRHIFormat(vk::Format format);

		VulkanGraphicsDevice* m_GraphicsDevice = nullptr;
		VulkanResourceManager* m_ResourceManager = nullptr;
		VulkanBindlessManager* m_BindlessManager = nullptr;

	};

}