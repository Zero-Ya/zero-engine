#include "VulkanBuffer.h"

#include "ZEngine/Core/Application.h"
#include "VulkanContext.h"
#include "VulkanCommandBuffer.h"

namespace ZEngine {
	uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);

	//

	VulkanVertexBuffer::VulkanVertexBuffer(float* vertices, uint32_t size) {
		auto vk_Context = static_cast<VulkanContext*>(Application::Get().GetGraphicsContext());
		auto& device = vk_Context->GetDevice();

		vk::BufferCreateInfo bufferInfo { .size = size, .usage = vk::BufferUsageFlagBits::eVertexBuffer, .sharingMode = vk::SharingMode::eExclusive };
		m_Buffer = vk::raii::Buffer(device, bufferInfo);

		vk::MemoryRequirements memRequirements = m_Buffer.getMemoryRequirements();
		vk::MemoryAllocateInfo allocInfo { .allocationSize = memRequirements.size, .memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent) };

		m_BufferMemory = vk::raii::DeviceMemory(device, allocInfo);
		m_Buffer.bindMemory(*m_BufferMemory, 0);

		void* data = m_BufferMemory.mapMemory(0, size);
		memcpy(data, vertices, static_cast<size_t>(size));
		m_BufferMemory.unmapMemory();
	}
	
	//

	// Index buffer
	VulkanIndexBuffer::VulkanIndexBuffer(uint32_t* indices, uint32_t count)
		: m_Count(count)
	{
		auto vk_Context = static_cast<VulkanContext*>(Application::Get().GetGraphicsContext());
		auto& device = vk_Context->GetDevice();
		uint32_t size = count * sizeof(uint32_t);

		vk::BufferCreateInfo bufferInfo{ .size = size, .usage = vk::BufferUsageFlagBits::eIndexBuffer, .sharingMode = vk::SharingMode::eExclusive };
		m_Buffer = vk::raii::Buffer(device, bufferInfo);

		vk::MemoryRequirements memRequirements = m_Buffer.getMemoryRequirements();
		vk::MemoryAllocateInfo allocInfo{ .allocationSize = memRequirements.size, .memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent) };

		m_BufferMemory = vk::raii::DeviceMemory(device, allocInfo);
		m_Buffer.bindMemory(*m_BufferMemory, 0);

		void* data = m_BufferMemory.mapMemory(0, size);
		memcpy(data, indices, static_cast<size_t>(size));
		m_BufferMemory.unmapMemory();
	}

	//

	// Uniform buffer
	VulkanUniformBuffer::VulkanUniformBuffer(uint32_t size, SetSlot setSlot, uint32_t binding, const Scope<LayoutManager>& layoutManager)
		: m_Size(size), m_SetSlot(setSlot), m_Binding(binding)
	{
		CreateDescriptorPool();

		auto vk_Context = static_cast<VulkanContext*>(Application::Get().GetGraphicsContext());
		auto& device = vk_Context->GetDevice();

		m_UniformBuffers.clear();
		m_UniformBuffersMemory.clear();
		m_UniformBuffersMapped.clear();

		for (size_t i = 0; i < vk_Context->GetMaxFramesInFlight(); i++) {
			vk::DeviceSize         bufferSize = size;
			vk::raii::Buffer       buffer({});
			vk::raii::DeviceMemory bufferMemory({});

			// Create the buffer
			vk::BufferCreateInfo bufferInfo{ .size = bufferSize, .usage = vk::BufferUsageFlagBits::eUniformBuffer, .sharingMode = vk::SharingMode::eExclusive };
			buffer = vk::raii::Buffer(device, bufferInfo);
			vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();
			vk::MemoryAllocateInfo allocInfo{ .allocationSize = memRequirements.size, .memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent) };
			bufferMemory = vk::raii::DeviceMemory(device, allocInfo);
			buffer.bindMemory(*bufferMemory, 0);

			m_UniformBuffers.emplace_back(std::move(buffer));
			m_UniformBuffersMemory.emplace_back(std::move(bufferMemory));
			m_UniformBuffersMapped.emplace_back(m_UniformBuffersMemory[i].mapMemory(0, bufferSize));
		}

		CreateDescriptorSets(layoutManager);
	}

	void VulkanUniformBuffer::SetData(const void* data, uint32_t size, uint32_t offset) {
		auto vk_Context = static_cast<VulkanContext*>(Application::Get().GetGraphicsContext());
		uint32_t currentFrame = vk_Context->GetCurrentFrameIndex();

		// Copy data into the active buffer
		uint8_t* destination = static_cast<uint8_t*>(m_UniformBuffersMapped[currentFrame]) + offset;
		std::memcpy(destination, data, size);
	}

	const vk::raii::DescriptorSet& VulkanUniformBuffer::GetFrameDescriptorSet() {
		auto vk_Context = static_cast<VulkanContext*>(Application::Get().GetGraphicsContext());
		uint32_t currentFrame = vk_Context->GetCurrentFrameIndex();

		return m_DescriptorSets[currentFrame];
	}

	// IDK Why I have this in uniform buffer class but uhhh this will do for now I guess
	void VulkanUniformBuffer::CreateDescriptorPool() {
		auto vk_Context = static_cast<VulkanContext*>(Application::Get().GetGraphicsContext());

		// Temporarily hardcoded, overcompensated
		auto maxFramesInFlight = vk_Context->GetMaxFramesInFlight();
		uint32_t maxMaterials = 10;
		uint32_t maxObjects = 10;

		uint32_t uboCount = (1 * maxFramesInFlight) + (1 * maxMaterials) + (1 * maxObjects);
		uint32_t samplerCount = (1 * maxFramesInFlight) + (1 * maxMaterials);

		std::array poolSize{
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, uboCount),
			vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, samplerCount)
		};

		uint32_t maxSets = maxFramesInFlight * 2 + maxMaterials + maxObjects;

		vk::DescriptorPoolCreateInfo poolInfo{
			.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
			.maxSets = maxSets,
			.poolSizeCount = static_cast<uint32_t>(poolSize.size()),
			.pPoolSizes = poolSize.data()
		};

		m_DescriptorPool = vk::raii::DescriptorPool(vk_Context->GetDevice(), poolInfo);
	}

	void VulkanUniformBuffer::CreateDescriptorSets(const Scope<LayoutManager>& layoutManager) {
		auto vk_Context = static_cast<VulkanContext*>(Application::Get().GetGraphicsContext());
		auto& device = vk_Context->GetDevice();

		auto g_LayoutManager = static_cast<VulkanLayoutManager*>(layoutManager.get());

		vk::DescriptorSetLayout targetLayout = g_LayoutManager->GetSetLayout(m_SetSlot);

		std::vector<vk::DescriptorSetLayout> layouts(vk_Context->GetMaxFramesInFlight(), targetLayout);
		vk::DescriptorSetAllocateInfo        allocInfo { .descriptorPool = m_DescriptorPool,
													     .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
													     .pSetLayouts = layouts.data() };

		m_DescriptorSets = device.allocateDescriptorSets(allocInfo);

		for (size_t i = 0; i < vk_Context->GetMaxFramesInFlight(); i++) {
			vk::DescriptorBufferInfo bufferInfo { .buffer = m_UniformBuffers[i], .offset = 0, .range = m_Size };
			vk::WriteDescriptorSet   descriptorWrite { .dstSet = m_DescriptorSets[i],
													   .dstBinding = m_Binding,
													   .dstArrayElement = 0,
													   .descriptorCount = 1,
													   .descriptorType = vk::DescriptorType::eUniformBuffer,
													   .pBufferInfo = &bufferInfo
													 };
			device.updateDescriptorSets(descriptorWrite, {});
		}
	}

	uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
		auto vk_Context = static_cast<VulkanContext*>(Application::Get().GetGraphicsContext());

		vk::PhysicalDeviceMemoryProperties memProperties = vk_Context->GetPhysicalDevice().getMemoryProperties();
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		ZE_CORE_ASSERT(false, "Failed to find suitable memory type for buffer allocation!");
		return 0;
	}

}