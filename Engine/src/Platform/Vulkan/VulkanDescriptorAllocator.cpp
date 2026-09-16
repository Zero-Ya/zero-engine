#include "VulkanDescriptorAllocator.h"

#include "ZEngine/Core/Application.h"
#include "VulkanContext.h"

namespace ZEngine {

	Scope<DescriptorAllocator> DescriptorAllocator::Create() {
		return std::make_unique<VulkanDescriptorAllocator>();
	}

	VulkanDescriptorAllocator::VulkanDescriptorAllocator() {
		CreateDescriptorPool();
	}

	void VulkanDescriptorAllocator::CreateDescriptorPool() {
		auto vk_Context = static_cast<VulkanContext*>(Application::Get().GetGraphicsContext());

		auto maxFramesInFlight = vk_Context->GetMaxFramesInFlight();
		uint32_t maxMaterials = 10000;
		uint32_t maxObjects = 10000;

		uint32_t uboCount = (1 * maxFramesInFlight) + (1 * maxMaterials) + (1 * maxObjects);
		uint32_t samplerCount = (1 * maxFramesInFlight) + (1 * maxMaterials);

		std::array poolSize {
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, uboCount),
			vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, samplerCount)
		};

		uint32_t maxSets = maxFramesInFlight * 2 + maxMaterials + maxObjects;

		vk::DescriptorPoolCreateInfo poolInfo {
			.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
			.maxSets = maxSets,
			.poolSizeCount = static_cast<uint32_t>(poolSize.size()),
			.pPoolSizes = poolSize.data()
		};

		m_DescriptorPool = vk::raii::DescriptorPool(vk_Context->GetDevice(), poolInfo);
	}

	vk::raii::DescriptorSet VulkanDescriptorAllocator::Allocate(SetSlot setSlot) {
		auto vk_Context = static_cast<VulkanContext*>(Application::Get().GetGraphicsContext());
		auto& device = vk_Context->GetDevice();

		auto vk_LayoutManager = static_cast<VulkanLayoutManager*>(vk_Context->GetLayoutManager().get());

		vk::DescriptorSetLayout targetLayout = vk_LayoutManager->GetSetLayout(setSlot);
		vk::DescriptorSetAllocateInfo        allocInfo{ .descriptorPool = m_DescriptorPool,
														.descriptorSetCount = 1,
														.pSetLayouts = &targetLayout };

		auto descriptorSets = device.allocateDescriptorSets(allocInfo);
		return std::move(descriptorSets[0]);
	}

	std::vector<vk::raii::DescriptorSet> VulkanDescriptorAllocator::AllocatePerFrames(SetSlot setSlot) {
		auto vk_Context = static_cast<VulkanContext*>(Application::Get().GetGraphicsContext());
		auto& device = vk_Context->GetDevice();

		auto vk_LayoutManager = static_cast<VulkanLayoutManager*>(vk_Context->GetLayoutManager().get());

		vk::DescriptorSetLayout targetLayout = vk_LayoutManager->GetSetLayout(setSlot);

		std::vector<vk::DescriptorSetLayout> layouts(vk_Context->GetMaxFramesInFlight(), targetLayout);
		vk::DescriptorSetAllocateInfo        allocInfo{ .descriptorPool = m_DescriptorPool,
														.descriptorSetCount = static_cast<uint32_t>(layouts.size()),
														.pSetLayouts = layouts.data() };

		return device.allocateDescriptorSets(allocInfo);
	}

}