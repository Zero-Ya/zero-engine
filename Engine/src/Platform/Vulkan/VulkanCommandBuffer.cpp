#include "VulkanCommandBuffer.h"

#include "ZEngine/Core/Application.h"
#include "VulkanGraphicsDevice.h"
#include "VulkanContext.h"

namespace ZEngine {

	VulkanCommandBuffer::VulkanCommandBuffer() {
		auto vk_GraphicsDevice = static_cast<VulkanGraphicsDevice*>(Application::Get().GetGraphicsDevice().get());
		auto& vk_Context = vk_GraphicsDevice->GetContext();
		auto& device = vk_Context.GetDevice();
		auto& commandPool = vk_Context.GetCurrentFrame().commandPool;

		const auto framesInFlight = vk_Context.MAX_FRAMES_IN_FLIGHT;

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
		auto vk_GraphicsDevice = static_cast<VulkanGraphicsDevice*>(Application::Get().GetGraphicsDevice().get());
		auto& vk_Context = vk_GraphicsDevice->GetContext();
		uint32_t activeFrameIndex = vk_Context.GetCurrentFrameIndex();

		return m_CommandBuffers[activeFrameIndex];
	}

}