#pragma once

#include "ZEngine/Renderer/Buffer.h"
#include "VulkanLayoutManager.h"

#include <vulkan/vulkan_raii.hpp>
#include <glm/glm.hpp>

namespace ZEngine {

	class VulkanVertexBuffer : public VertexBuffer {
	public:
		VulkanVertexBuffer(float* vertices, uint32_t size);
		virtual ~VulkanVertexBuffer() override = default;

		virtual void Bind() const override {}
		virtual void Unbind() const override {}

		virtual const BufferLayout& GetLayout() const override { return m_Layout; }
		virtual void SetLayout(const BufferLayout& layout) override { m_Layout = layout; }

		const vk::raii::Buffer& GetNativeHandle() { return m_Buffer; }

	private:
		BufferLayout m_Layout;
		vk::raii::Buffer m_Buffer = nullptr;
		vk::raii::DeviceMemory m_BufferMemory = nullptr;
	};

	class VulkanIndexBuffer : public IndexBuffer {
	public:
		VulkanIndexBuffer(uint32_t* indices, uint32_t count);
		virtual ~VulkanIndexBuffer() override = default;

		virtual void Bind() const override {}
		virtual void Unbind() const override {}

		virtual uint32_t GetCount() const override { return m_Count; }
		const vk::raii::Buffer& GetNativeHandle() { return m_Buffer; }

	private:
		uint32_t m_Count = 0;
		vk::raii::Buffer m_Buffer = nullptr;
		vk::raii::DeviceMemory m_BufferMemory = nullptr;
	};

	class VulkanUniformBuffer : public UniformBuffer {
	public:
		VulkanUniformBuffer(uint32_t size, uint32_t setSlot, uint32_t binding, const std::unique_ptr<LayoutManager>& layoutManager);
		virtual ~VulkanUniformBuffer() override = default;

		virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) override;

		const std::vector<vk::raii::Buffer>& GetUniformBuffers() { return m_UniformBuffers; }
		const std::vector<vk::raii::DescriptorSet>& GetDescriptorSets() { return m_DescriptorSets; }
		const vk::raii::DescriptorSet& GetFrameDescriptorSet();

	private:
		void CreateDescriptorPool();
		void CreateDescriptorSets(const std::unique_ptr<LayoutManager>& layoutManager);

	private:
		uint32_t m_Size = 0;
		uint32_t m_SetSlot = 0;
		uint32_t m_Binding = 0;
		std::unique_ptr<LayoutManager> m_LayoutManager;

		vk::raii::DescriptorPool             m_DescriptorPool = nullptr;
		std::vector<vk::raii::DescriptorSet> m_DescriptorSets;

		std::vector<vk::raii::Buffer>        m_UniformBuffers;
		std::vector<vk::raii::DeviceMemory>  m_UniformBuffersMemory;
		std::vector<void*>                   m_UniformBuffersMapped;
	};

}