#pragma once

#include "ZEngine/Asset/ShaderTypes.h"
#include "Vertex.h"

namespace ZEngine {

    // Represents a single primitive/submesh/draw call with geometry buffers and PBR materials
    struct Submesh {
        BufferHandle vertexBuffer;
        BufferHandle indexBuffer;
        uint32_t indexCount = 0;
        uint32_t vertexCount = 0;

        MaterialData material;
    };

    // A Mesh contains multiple primitives (submeshes)
    struct Mesh {
        std::string name;
        std::vector<Submesh> primitives;
    };

    // Represents an item in the transform hierarchy tree
    struct Node {
        std::string name;
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

    // Container asset holding meshes, textures, and root nodes of the scene graph
    struct Model {
        std::string name;
        std::vector<Mesh> meshes;
        std::vector<std::shared_ptr<Node>> rootNodes;

        std::vector<TextureHandle> textures;
    };

}