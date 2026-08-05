#include "VertexArray.h"
#include "Renderer.h"

#include "Platform/Vulkan/VulkanVertexArray.h"

namespace ZEngine {

    std::shared_ptr<VertexArray> VertexArray::Create() {
        switch (Renderer::GetAPI()) {
            case RendererAPI::API::None:
                ZE_CORE_ASSERT(false, "RendererAPI::None is currently unsupported!");
                return nullptr;
            case RendererAPI::API::Vulkan:
                return std::make_shared<VulkanVertexArray>();
            }
        ZE_CORE_ASSERT(false, "Unknown RendererAPI backend encountered during VertexArray allocation!");
        return nullptr;
    }

}