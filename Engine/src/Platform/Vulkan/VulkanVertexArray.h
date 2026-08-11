#pragma once

#include "ZEngine/Renderer/VertexArray.h"

#include <vulkan/vulkan_raii.hpp>

namespace ZEngine {

    class VulkanVertexArray : public VertexArray {
    public:
        VulkanVertexArray() = default;
        virtual ~VulkanVertexArray() override = default;

        virtual void Bind() const override {};
        virtual void Unbind() const override {};

        virtual void SetVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) override;
        virtual void SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer) override;

        virtual const Ref<VertexBuffer>& GetVertexBuffer() const override { return m_VertexBuffer; }
        virtual const Ref<IndexBuffer>& GetIndexBuffer() const override { return m_IndexBuffer; }

        // Vulkan bind
        void BindToCommandBuffer(const vk::raii::CommandBuffer& cmdBuffer) const;

    private:
        Ref<VertexBuffer> m_VertexBuffer;
        Ref<IndexBuffer> m_IndexBuffer;
    };

}