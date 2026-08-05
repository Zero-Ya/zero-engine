#pragma once

#include "ZEngine/Renderer/RenderCommandBuffer.h"

#include <vulkan/vulkan_raii.hpp>

namespace ZEngine {

	class VulkanCommandBuffer : public RenderCommandBuffer {
	public:
		VulkanCommandBuffer();
		virtual ~VulkanCommandBuffer() override = default;

		virtual void Begin() override;
		virtual void End() override;
		virtual void Reset() override;

		const std::vector<vk::raii::CommandBuffer>& GetBuffers() { return m_CommandBuffers; };
		const vk::raii::CommandBuffer& GetBuffer() const;

	private:
		std::vector<vk::raii::CommandBuffer> m_CommandBuffers;
	};

}