#pragma once

#include "ZEngine/Renderer/Model.h"

#include <vulkan/vulkan_raii.hpp>

#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace ZEngine {

    class Texture2D;
    class Material;
    class VertexBuffer;
    class IndexBuffer;
    struct Vertex;

    struct Primitive {
        uint32_t firstIndex;
        uint32_t indexCount;
        int32_t materialIndex = -1;
    };

    struct Mesh {
        std::vector<Primitive> primitives;
    };

    struct Node {
        std::weak_ptr<Node> parent;
        std::vector<std::shared_ptr<Node>> children;

        int32_t meshIndex = -1;
        glm::mat4 localMatrix{ 1.0f };

        glm::mat4 GetGlobalMatrix() const {
            if (auto p = parent.lock()) {
                return p->GetGlobalMatrix() * localMatrix;
            }
            return localMatrix;
        }
    };

    class VulkanModel : public Model {
    public:
        VulkanModel() = default;
        ~VulkanModel() = default;

        bool LoadFromFile(const std::string& filePath) override;
        void Draw(const Ref<RenderCommandBuffer>& commandBuffer, const Ref<PipelineState>& pipelineState) override;

    private:
        void LoadTexture(const std::string& baseDir);
        void LoadMaterial();
        void LoadMesh(std::vector<Vertex>& allVertices, std::vector<uint32_t>& allIndices);

        void DrawNode(vk::CommandBuffer cmd, vk::PipelineLayout pipelineLayout, const Node& node);

        // Single contiguous geometry allocation
        Ref<VertexBuffer> m_VertexBuffer;
        Ref<IndexBuffer> m_IndexBuffer;

        fastgltf::Asset m_Asset;
        std::vector<Mesh> m_Meshes;
        std::vector<std::shared_ptr<Node>> m_RootNodes;

        std::vector<Ref<Texture2D>> m_Textures;
        std::vector<Ref<Material>> m_Materials;
    };

}