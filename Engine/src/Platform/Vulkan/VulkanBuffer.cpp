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
	VulkanUniformBuffer::VulkanUniformBuffer(uint32_t size, uint32_t setSlot, uint32_t binding, const std::unique_ptr<LayoutManager>& layoutManager)
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

	void VulkanUniformBuffer::CreateDescriptorPool() {
		auto vk_Context = static_cast<VulkanContext*>(Application::Get().GetGraphicsContext());

		std::array poolSize{
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, vk_Context->GetMaxFramesInFlight()),
			vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, vk_Context->GetMaxFramesInFlight())
		};

		vk::DescriptorPoolCreateInfo poolInfo{
			.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
			.maxSets = vk_Context->GetMaxFramesInFlight(),
			.poolSizeCount = static_cast<uint32_t>(poolSize.size()),
			.pPoolSizes = poolSize.data()
		};

		m_DescriptorPool = vk::raii::DescriptorPool(vk_Context->GetDevice(), poolInfo);
	}

	void VulkanUniformBuffer::CreateDescriptorSets(const std::unique_ptr<LayoutManager>& layoutManager) {
		auto vk_Context = static_cast<VulkanContext*>(Application::Get().GetGraphicsContext());
		auto& device = vk_Context->GetDevice();

		auto g_LayoutManager = static_cast<VulkanLayoutManager*>(layoutManager.get());

		vk::DescriptorSetLayout targetLayout = g_LayoutManager->GetDescriptorSetLayout(m_SetSlot);

		std::vector<vk::DescriptorSetLayout> layouts(vk_Context->GetMaxFramesInFlight(), targetLayout);
		vk::DescriptorSetAllocateInfo        allocInfo { .descriptorPool = m_DescriptorPool,
													     .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
													     .pSetLayouts = layouts.data() };

		m_DescriptorSets = device.allocateDescriptorSets(allocInfo);

		for (size_t i = 0; i < vk_Context->GetMaxFramesInFlight(); i++) {
			vk::DescriptorBufferInfo bufferInfo { .buffer = m_UniformBuffers[i], .offset = 0, .range = m_Size };
			vk::WriteDescriptorSet   descriptorWrite { .dstSet = m_DescriptorSets[i],
													   .dstBinding = 0,
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