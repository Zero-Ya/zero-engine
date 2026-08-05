#include "VulkanCommandBuffer.h"
#include "ZEngine/Core/Application.h"
#include "Platform/Vulkan/VulkanContext.h"

namespace ZEngine {

	VulkanCommandBuffer::VulkanCommandBuffer() {
		auto vk_Context = static_cast<VulkanContext*>(Application::Get().GetGraphicsContext());
		auto& device = vk_Context->GetDevice();
		auto& commandPool = vk_Context->GetCommandPool();

		const auto framesInFlight = vk_Context->GetMaxFramesInFlight();

		vk::CommandBufferAllocateInfo allocInfo;
		allocInfo.commandPool = *commandPool;
		allocInfo.level = vk::CommandBufferLevel::ePrimary;
		allocInfo.commandBufferCount = framesInFlight;
		m_CommandBuffers = vk::raii::CommandBuffers(device, allocInfo);
	}

	void VulkanCommandBuffer::Begin() {
		vk::CommandBufferBeginInfo beginInfo;
		beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

		GetBuffer().begin(beginInfo);
	}

	void VulkanCommandBuffer::End() {
		GetBuffer().end();
	}

	void VulkanCommandBuffer::Reset() {
		GetBuffer().reset();
	}

	const vk::raii::CommandBuffer& VulkanCommandBuffer::GetBuffer() const {
		auto vk_Context = static_cast<VulkanContext*>(Application::Get().GetGraphicsContext());
		uint32_t activeFrameIndex = vk_Context->GetCurrentFrameIndex();

		return m_CommandBuffers[activeFrameIndex];
	}

}