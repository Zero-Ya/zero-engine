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

        virtual void SetVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexBuffer) override;
        virtual void SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer) override;

        virtual const std::shared_ptr<VertexBuffer>& GetVertexBuffer() const override { return m_VertexBuffer; }
        virtual const std::shared_ptr<IndexBuffer>& GetIndexBuffer() const override { return m_IndexBuffer; }

        // Vulkan bind
        void BindToCommandBuffer(const vk::raii::CommandBuffer& cmdBuffer) const;

    private:
        std::shared_ptr<VertexBuffer> m_VertexBuffer;
        std::shared_ptr<IndexBuffer> m_IndexBuffer;
    };

}