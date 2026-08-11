#include "VulkanVertexArray.h"
#include "Platform/Vulkan/VulkanBuffer.h"

namespace ZEngine {

    void VulkanVertexArray::SetVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) {
        m_VertexBuffer = vertexBuffer;
    }

    void VulkanVertexArray::SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer) {
        m_IndexBuffer = indexBuffer;
    }

    void VulkanVertexArray::BindToCommandBuffer(const vk::raii::CommandBuffer& cmdBuffer) const {
        auto& rawVertexBuffer = static_cast<VulkanVertexBuffer*>(m_VertexBuffer.get())->GetNativeHandle();
        auto& rawIndexBuffer = static_cast<VulkanIndexBuffer*>(m_IndexBuffer.get())->GetNativeHandle();

        cmdBuffer.bindVertexBuffers(0, *rawVertexBuffer, { 0 });
        cmdBuffer.bindIndexBuffer(*rawIndexBuffer, 0, vk::IndexType::eUint32);
    }

}