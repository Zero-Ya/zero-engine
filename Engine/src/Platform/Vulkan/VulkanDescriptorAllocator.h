#pragma once

#include "ZEngine/Renderer/DescriptorAllocator.h"
#include "VulkanLayoutManager.h"

#include <vulkan/vulkan_raii.hpp>

namespace ZEngine {

	class VulkanDescriptorAllocator : public DescriptorAllocator {
	public:
		VulkanDescriptorAllocator();
		~VulkanDescriptorAllocator() override = default;

		void Clear() override {};

		// Single set allocation
		vk::raii::DescriptorSet Allocate(SetSlot setSlot);
		// Frame-in-flight sets allocation
		std::vector<vk::raii::DescriptorSet> AllocatePerFrames(SetSlot setSlot);

	private:
		void CreateDescriptorPool();

	private:
		vk::raii::DescriptorPool m_DescriptorPool = nullptr;
	};

}