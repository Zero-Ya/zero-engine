#include "Buffer.h"
#include "Renderer.h"
#include "Platform/Vulkan/VulkanBuffer.h"

namespace ZEngine {

	Ref<VertexBuffer> VertexBuffer::Create(float* vertices, uint32_t size) {
		switch (Renderer::GetAPI()) {
			case RendererAPI::API::None:    ZE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::Vulkan:  return std::make_shared<VulkanVertexBuffer>(vertices, size);
		}

		ZE_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref<IndexBuffer> IndexBuffer::Create(uint32_t* indices, uint32_t size) {
		switch (Renderer::GetAPI()) {
			case RendererAPI::API::None:    ZE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::Vulkan:  return std::make_shared<VulkanIndexBuffer>(indices, size);
		}

		ZE_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref<UniformBuffer> UniformBuffer::Create(size_t size) {
		switch (Renderer::GetAPI()) {
			case RendererAPI::API::None:    ZE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::Vulkan:  return std::make_shared<VulkanUniformBuffer>(size);
		}

		ZE_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}