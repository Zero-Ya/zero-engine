#pragma once

#include "VulkanContext.h"
#include "VulkanSwapchain.h"
#include "ZEngine/Renderer/ResourceManager.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace ZEngine {

	class ZE_API VulkanPipelineManager {
	public:
		VulkanPipelineManager(VulkanContext* ctx, VulkanSwapchain* swapchain, ResourceManager* resource);
		~VulkanPipelineManager();

		vk::raii::Pipeline& getGraphicsPipeline() { return graphicsPipeline; };
		vk::raii::PipelineLayout& getLayout() { return pipelineLayout; };

		std::vector<vk::raii::DescriptorSet>& getDescriptorSets() { return descriptorSets; };

		void createGraphicsPipeline();

		void createDescriptorSetLayout();
		void createDescriptorPool();
		void createDescriptorSets();

	private:
		VulkanContext*						 vk_Ctx;
		VulkanSwapchain*					 vk_Swapchain;
		ResourceManager*					 resourceManager;

		vk::raii::DescriptorSetLayout		 descriptorSetLayout = nullptr;
		vk::raii::PipelineLayout			 pipelineLayout = nullptr;
		vk::raii::Pipeline					 graphicsPipeline = nullptr;

		vk::raii::DescriptorPool             descriptorPool = nullptr;
		std::vector<vk::raii::DescriptorSet> descriptorSets;

		vk::raii::ShaderModule createShaderModule(const std::vector<char>& code) const;
	};

}