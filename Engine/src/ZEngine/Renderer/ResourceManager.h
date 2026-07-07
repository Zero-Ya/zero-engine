#pragma once

#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanSwapchain.h"
#include "Platform/Vulkan/VulkanCommandManager.h"

#include <stb_image.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace ZEngine {
	class VulkanSwapchain;

	struct Vertex
	{
		glm::vec3 pos;
		glm::vec3 color;
		glm::vec2 texCoord;

		static vk::VertexInputBindingDescription getBindingDescription() {
			return { 0, sizeof(Vertex), vk::VertexInputRate::eVertex };
		}

		static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions() {
			return {
				vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)),
				vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)),
				vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord))
			};
		}
	};

	struct UniformBufferObject
	{
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};

	const std::vector<Vertex> vertices = {
		{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
		{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
		{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

		{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
		{{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
		{{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
	};

	const std::vector<uint16_t> indices = {
		0, 1, 2, 2, 3, 0,
		4, 5, 6, 6, 7, 4
	};

	class ZE_API ResourceManager {
	public:
		ResourceManager(VulkanContext* ctx, VulkanCommandManager* command, VulkanSwapchain* swapchain);
		~ResourceManager();

		void init()
		{
			createDepthResources();
			createTextureImage();
			createTextureImageView();
			createTextureSampler();
			createVertexBuffer();
			createIndexBuffer();
			createUniformBuffers();
		};

		vk::raii::Image& getDepthImage() { return depthImage; };
		vk::raii::ImageView& getDepthImageView() { return depthImageView; };

		vk::raii::Image& getTextureImage() { return textureImage; };
		vk::raii::ImageView& getTextureImageView() { return textureImageView; };
		vk::raii::Sampler& getTextureSampler() { return textureSampler; };

		vk::raii::Buffer& getVertexBuffer() { return vertexBuffer; };
		vk::raii::Buffer& getIndexBuffer() { return indexBuffer; };

		std::vector<vk::raii::Buffer>& getUniformBuffers() { return uniformBuffers; };
		std::vector<void*>& getUniformBuffersMapped() { return uniformBuffersMapped; };

		void createDepthResources();

		// Useful helper functions //

		void transitionImageLayout(const vk::raii::Image& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
		uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);

		void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Buffer& buffer, vk::raii::DeviceMemory& bufferMemory);
		void copyBuffer(vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::DeviceSize size);
		void createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Image& image, vk::raii::DeviceMemory& imageMemory);
		std::unique_ptr<vk::raii::CommandBuffer> beginSingleTimeCommands();
		void endSingleTimeCommands(vk::raii::CommandBuffer& commandBuffer);
		void copyBufferToImage(const vk::raii::Buffer& buffer, vk::raii::Image& image, uint32_t width, uint32_t height);
		vk::raii::ImageView createImageView(vk::raii::Image& image, vk::Format format, vk::ImageAspectFlags aspectFlags);

		vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
		vk::Format findDepthFormat();
		bool hasStencilComponent(vk::Format format);

	private:
		VulkanContext* vk_Ctx = nullptr;
		VulkanSwapchain* vk_Swapchain = nullptr;
		VulkanCommandManager* vk_CommandManager = nullptr;

		vk::raii::Image						 depthImage = nullptr;
		vk::raii::DeviceMemory				 depthImageMemory = nullptr;
		vk::raii::ImageView					 depthImageView = nullptr;

		vk::raii::Image						 textureImage = nullptr;
		vk::raii::DeviceMemory				 textureImageMemory = nullptr;
		vk::raii::ImageView					 textureImageView = nullptr;
		vk::raii::Sampler					 textureSampler = nullptr;

		vk::raii::Buffer					 vertexBuffer = nullptr;
		vk::raii::DeviceMemory				 vertexBufferMemory = nullptr;
		vk::raii::Buffer					 indexBuffer = nullptr;
		vk::raii::DeviceMemory				 indexBufferMemory = nullptr;

		std::vector<vk::raii::Buffer>        uniformBuffers;
		std::vector<vk::raii::DeviceMemory>  uniformBuffersMemory;
		std::vector<void*>                   uniformBuffersMapped;

		void createTextureImage();
		void createTextureImageView();
		void createTextureSampler();
		void createVertexBuffer();
		void createIndexBuffer();
		void createUniformBuffers();
	};

}