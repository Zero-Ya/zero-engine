#pragma once

#include "VulkanContext.h"
#include "VulkanSwapchain.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace ZEngine {

	struct Vertex
	{
		glm::vec2 pos;
		glm::vec3 color;
		glm::vec2 texCoord;

		static vk::VertexInputBindingDescription getBindingDescription() {
			return { 0, sizeof(Vertex), vk::VertexInputRate::eVertex };
		}

		static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions() {
			return {
				vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, pos)),
				vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)),
				vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord))
			};
		}
	};

	class ZE_API VulkanPipelineManager {
	public:
		VulkanPipelineManager(VulkanContext* ctx, VulkanSwapchain* swapchain);
		~VulkanPipelineManager();

		vk::raii::Pipeline& getGraphicsPipeline() { return graphicsPipeline; };

		void createGraphicsPipeline();

	private:
		VulkanContext*						 vk_Ctx;
		VulkanSwapchain*					 vk_Swapchain;
											 
		vk::raii::DescriptorSetLayout		 descriptorSetLayout = nullptr;
		vk::raii::PipelineLayout			 pipelineLayout = nullptr;
		vk::raii::Pipeline					 graphicsPipeline = nullptr;

		vk::raii::DescriptorPool             descriptorPool = nullptr;
		std::vector<vk::raii::DescriptorSet> descriptorSets;

		void createDescriptorSetLayout();
		void createDescriptorPool();
		void createDescriptorSets();
		vk::raii::ShaderModule createShaderModule(const std::vector<char>& code) const;
	};

}