#include "VulkanModel.h"

#include "ZEngine/Core/Application.h"
#include "VulkanContext.h"

#include <ktx.h>
#include <ktxvulkan.h>

#include "VulkanBuffer.h"
#include "VulkanTexture.h"
#include "VulkanMaterial.h"
#include "VulkanCommandBuffer.h"
#include "VulkanPipelineState.h"
#include "Platform/Vulkan/VulkanDescriptorAllocator.h"

namespace {

    std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> CreateBuffer(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
    uint32_t FindMemoryType(vk::raii::PhysicalDevice physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties);
    void CopyBuffer(vk::raii::CommandBuffer& commandBuffer, const vk::raii::Buffer& srcBuffer, const vk::raii::Buffer& dstBuffer, vk::DeviceSize size);

    vk::raii::CommandBuffer BeginSingleTimeCommands(const vk::raii::Device& device, const vk::raii::CommandPool& commandPool);
    void EndSingleTimeCommands(vk::raii::CommandBuffer&& commandBuffer, const vk::raii::Queue queue);

}

namespace ZEngine {

    bool VulkanModel::LoadFromFile(const std::string& filePath) {
        auto vk_Context = static_cast<VulkanContext*>(Application::Get().GetGraphicsContext());
        auto& device = vk_Context->GetDevice();
        auto& physicalDevice = vk_Context->GetPhysicalDevice();

        fastgltf::Parser parser(fastgltf::Extensions::KHR_texture_basisu);

        auto data = fastgltf::GltfDataBuffer::FromPath(filePath);
        if (data.error() != fastgltf::Error::None) {
            std::cerr << "Failed to open glTF file: " << filePath << std::endl;
            return false;
        }

        // Determine parent directory for loading external resources (buffers/images)
        std::string pathStr = filePath;
        std::string baseDir = pathStr.substr(0, pathStr.find_last_of("/\\") + 1);

        constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember |
            fastgltf::Options::LoadExternalBuffers;

        auto assetResult = parser.loadGltfBinary(data.get(), baseDir, gltfOptions);
        if (assetResult.error() != fastgltf::Error::None) {
            // Fallback to ASCII .gltf parsing if glb parsing fails
            assetResult = parser.loadGltf(data.get(), baseDir, gltfOptions);
            if (assetResult.error() != fastgltf::Error::None) {
                std::cerr << "Failed to parse glTF asset: " << static_cast<uint64_t>(assetResult.error()) << std::endl;
                return false;
            }
        }

        m_Asset = std::move(assetResult.get());

        std::vector<Vertex> allVertices {};
        std::vector<uint32_t> allIndices {};

        LoadTexture(baseDir);
        LoadMaterial();
        LoadMesh(allVertices, allIndices);

        vk::DeviceSize vertexSize = allVertices.size() * sizeof(Vertex);
        vk::DeviceSize indexSize = allIndices.size() * sizeof(uint32_t) / sizeof(uint32_t);

        // Allocate single contiguous GPU buffers for entire model
        m_VertexBuffer = VertexBuffer::Create(allVertices, vertexSize);
        m_IndexBuffer = IndexBuffer::Create(allIndices, indexSize);

        auto rawVertexBuffer = static_cast<VulkanVertexBuffer*>(m_VertexBuffer.get());
        auto rawIndexBuffer = static_cast<VulkanIndexBuffer*>(m_IndexBuffer.get());

        // Staging transfers from CPU -> GPU buffers...
        auto [stagingBuffer, stagingBufferMemory] =
            CreateBuffer(device, physicalDevice, allVertices.size() * sizeof(Vertex), vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        // Create a helper function for this later...
        void* stagingData = stagingBufferMemory.mapMemory(0, vertexSize);
        memcpy(stagingData, allVertices.data(), vertexSize);
        stagingBufferMemory.unmapMemory();

        vk::raii::CommandBuffer commandBuffer = BeginSingleTimeCommands(device, vk_Context->GetCommandPool());
        CopyBuffer(commandBuffer, stagingBuffer, rawVertexBuffer->GetNativeHandle(), vertexSize);
        EndSingleTimeCommands(std::move(commandBuffer), vk_Context->GetGraphicsQueue());

        // Parse Scene Node Hierarchy
        // (Builds parent/child links and root nodes)
        std::vector<std::shared_ptr<Node>> allNodes(m_Asset.nodes.size());
        for (size_t i = 0; i < m_Asset.nodes.size(); ++i) {
            allNodes[i] = std::make_shared<Node>();
        }

        for (size_t i = 0; i < m_Asset.nodes.size(); ++i) {
            auto& srcNode = m_Asset.nodes[i];
            auto& dstNode = allNodes[i];

            if (srcNode.meshIndex.has_value()) {
                dstNode->meshIndex = static_cast<int32_t>(srcNode.meshIndex.value());
            }

            // Get local matrix or decompose translation/rotation/scale
            std::visit(fastgltf::visitor{
                [&](const fastgltf::math::fmat4x4& matrix) {
                    dstNode->localMatrix = glm::make_mat4(matrix.data());
                },
                [&](const fastgltf::TRS& trs) {
                    glm::mat4 T = glm::translate(glm::mat4(1.0f), glm::make_vec3(trs.translation.data()));
                    glm::quat R = glm::make_quat(trs.rotation.data());
                    glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::make_vec3(trs.scale.data()));
                    dstNode->localMatrix = T * glm::mat4_cast(R) * S;
                }
                }, srcNode.transform);

            // Assign hierarchy links
            for (auto childIdx : srcNode.children) {
                dstNode->children.push_back(allNodes[childIdx]);
                allNodes[childIdx]->parent = dstNode;
            }
        }

        // Collect Root Nodes from the default scene
        const auto& scene = m_Asset.scenes[m_Asset.defaultScene.value_or(0)];
        for (auto nodeIdx : scene.nodeIndices) {
            m_RootNodes.push_back(allNodes[nodeIdx]);
        }

        return true;
    }

    void VulkanModel::DrawNode(vk::CommandBuffer cmd, vk::PipelineLayout pipelineLayout, const Node& node) {
        // Process mesh primitives attached to this node
        if (node.meshIndex != -1) {
            // Compute and push the accumulated global matrix (World Transform)
            glm::mat4 globalMatrix = node.GetGlobalMatrix();
            cmd.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(glm::mat4), glm::value_ptr(globalMatrix));

            const Mesh& mesh = m_Meshes[node.meshIndex];

            for (const Primitive& prim : mesh.primitives) {
                // Bind material/texture descriptor set
                if (prim.materialIndex >= 0 && prim.materialIndex < m_Textures.size()) {
                    const auto& mat = static_cast<VulkanMaterial*>(m_Materials[prim.materialIndex].get());
                    auto& descriptorSet = mat->GetDescriptorSet();

                    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, static_cast<uint32_t>(SetSlot::Material), *descriptorSet, nullptr);
                }

                // Issue indexed draw call using submesh offsets
                cmd.drawIndexed(prim.indexCount, 1, prim.firstIndex, 0, 0);
            }
        }

        // Recurse down to child nodes
        for (const auto& child : node.children) {
            DrawNode(cmd, pipelineLayout, *child);
        }
    }

    void VulkanModel::Draw(const Ref<RenderCommandBuffer>& commandBuffer, const Ref<PipelineState>& pipelineState) {
        auto vulkanCommandBuffer = static_cast<VulkanCommandBuffer*>(commandBuffer.get());
        const auto& cmd = vulkanCommandBuffer->GetBuffer();

        auto vulkanPipeline = static_cast<VulkanPipelineState*>(pipelineState.get());
        auto& pipelineLayout = vulkanPipeline->GetRawNativeLayout();

        auto& rawVertexBuffer = static_cast<VulkanVertexBuffer*>(m_VertexBuffer.get())->GetNativeHandle();
        auto& rawIndexBuffer = static_cast<VulkanIndexBuffer*>(m_IndexBuffer.get())->GetNativeHandle();

        if (!m_VertexBuffer || !m_IndexBuffer) {
            return;
        }

        // Bind global continuous vertex buffer once for all nodes
        vk::Buffer vertexBuffers[] = { *rawVertexBuffer };
        vk::DeviceSize offsets[] = { 0 };
        cmd.bindVertexBuffers(0, vertexBuffers, offsets);

        // Bind global index buffer once
        cmd.bindIndexBuffer(*rawIndexBuffer, 0, vk::IndexType::eUint32);

        // Traverse all root nodes in the scene hierarchy
        for (const auto& rootNode : m_RootNodes) {
            DrawNode(cmd, pipelineLayout, *rootNode);
        }
    }

    void VulkanModel::LoadTexture(const std::string& baseDir) {
        m_Textures.reserve(m_Asset.textures.size());

        for (const auto& gltfTexture : m_Asset.textures) {
            if (!gltfTexture.imageIndex.has_value()) continue;

            const auto& gltfImage = m_Asset.images[gltfTexture.imageIndex.value()];

            std::string texturePath;
            std::visit(fastgltf::visitor{
                [&](const fastgltf::sources::URI& filePathURI) {
                    texturePath = baseDir + "/" + std::string(filePathURI.uri.path().begin(), filePathURI.uri.path().end());;
                },
                [&](const fastgltf::sources::Vector& vec) {
                    // If embedded, load from memory buffer directly
                },
                [](const auto&) {}
                }, gltfImage.data);

            // Call KTX Texture Loader
            Ref<Texture2D> tex = Texture2D::Create();
            tex->LoadTexture(texturePath);
            m_Textures.push_back(std::move(tex));
        }
    }

    void VulkanModel::LoadMaterial() {
        auto vk_Context = static_cast<VulkanContext*>(Application::Get().GetGraphicsContext());
        auto& device = vk_Context->GetDevice();
        auto& physicalDevice = vk_Context->GetPhysicalDevice();

        m_Materials.reserve(m_Asset.materials.size());

        for (const auto& gltfMaterial : m_Asset.materials) {
            Ref<Material> refMaterial = Material::Create(gltfMaterial.name.c_str());
            auto vk_Material = static_cast<VulkanMaterial*>(refMaterial.get());

            vk_Material->SetAlbedoColor(glm::make_vec4(gltfMaterial.pbrData.baseColorFactor.data()));
            vk_Material->SetMetallic(gltfMaterial.pbrData.metallicFactor);
            vk_Material->SetRoughness(gltfMaterial.pbrData.roughnessFactor);

            vk_Material->Init();

            // Copy material factors into UBO
            vk_Material->UpdateBuffer();

            vk_Material->AllocateDescriptorSet();

            // Determine Albedo Texture Index
            uint32_t textureIndex = 0; // Default to first loaded texture
            if (gltfMaterial.pbrData.baseColorTexture.has_value()) {
                textureIndex = static_cast<uint32_t>(gltfMaterial.pbrData.baseColorTexture->textureIndex);
            }

            const Ref<Texture2D>& refTex = m_Textures[textureIndex];
            const auto& targetTex = static_cast<VulkanTexture2D*>(refTex.get());

            vk_Material->SetAlbedoTexture(refTex);

            vk_Material->UpdateDescriptorSets();

            m_Materials.push_back(std::move(refMaterial));
        }
    }

    void VulkanModel::LoadMesh(std::vector<Vertex>& allVertices, std::vector<uint32_t>& allIndices) {
        m_Meshes.reserve(m_Asset.meshes.size());
        // Load Geometry and pack into continuous staging vectors
        for (auto& mesh : m_Asset.meshes) {
            Mesh outMesh{};

            for (auto& prim : mesh.primitives) {
                Primitive outPrim;
                outPrim.firstIndex = static_cast<uint32_t>(allIndices.size());
                uint32_t vertexStart = static_cast<uint32_t>(allVertices.size());
                
                // Extract position
                auto posIt = prim.findAttribute("POSITION");
                if (posIt == prim.attributes.end()) {
                    continue; // Primitive has no positions; skip
                }

                auto& posAccessor = m_Asset.accessors[posIt->accessorIndex];
                size_t vertexCount = posAccessor.count;
                allVertices.resize(vertexStart + vertexCount);

                fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(m_Asset, posAccessor, [&](fastgltf::math::fvec3 element, size_t index) {
                    size_t targetIndex = vertexStart + index;
                    allVertices[targetIndex].position = glm::vec3(element[0], element[1], element[2]);
                    // Set safe fallback defaults for optional attributes
                    allVertices[targetIndex].normal = glm::vec3(0.0f, 1.0f, 0.0f);
                    allVertices[targetIndex].uv = glm::vec2(0.0f, 0.0f);
                    }
                );

                // Extract normal
                auto normIt = prim.findAttribute("NORMAL");
                if (normIt != prim.attributes.end()) {
                    auto& normAccessor = m_Asset.accessors[normIt->accessorIndex];
                    fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(m_Asset, normAccessor, [&](fastgltf::math::fvec3 element, size_t index) {
                        allVertices[vertexStart + index].normal = glm::vec3(element[0], element[1], element[2]);
                        }
                    );
                }

                // Extract texture coords 0
                auto uvIt = prim.findAttribute("TEXCOORD_0");
                if (uvIt != prim.attributes.end()) {
                    auto& uvAccessor = m_Asset.accessors[uvIt->accessorIndex];
                    fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(m_Asset, uvAccessor, [&](fastgltf::math::fvec2 element, size_t index) {
                        allVertices[vertexStart + index].uv = glm::vec2(element[0], element[1]);
                        }
                    );
                }

                // Extract indices
                if (prim.indicesAccessor.has_value()) {
                    auto& indexAccessor = m_Asset.accessors[prim.indicesAccessor.value()];
                    allIndices.reserve(allIndices.size() + indexAccessor.count);

                    fastgltf::iterateAccessor<std::uint32_t>(
                        m_Asset, indexAccessor, [&](std::uint32_t index) {
                            // Offset by vertexStart to account for shared single vertex buffer
                            allIndices.push_back(vertexStart + index);
                        }
                    );

                    outPrim.indexCount = static_cast<uint32_t>(indexAccessor.count);
                }
                else {
                    // If no index buffer is present, generate sequential indices (0, 1, 2, ...)
                    for (uint32_t i = 0; i < vertexCount; ++i) {
                        allIndices.push_back(vertexStart + i);
                    }
                    outPrim.indexCount = static_cast<uint32_t>(vertexCount);
                }

                outPrim.materialIndex = static_cast<int32_t>(prim.materialIndex.value_or(-1));
                outMesh.primitives.push_back(outPrim);
            }
            m_Meshes.push_back(outMesh);
        }
    }

}

namespace {

    std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> CreateBuffer(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties) {
        vk::BufferCreateInfo   bufferInfo{ .size = size, .usage = usage, .sharingMode = vk::SharingMode::eExclusive };
        vk::raii::Buffer       buffer = vk::raii::Buffer(device, bufferInfo);
        vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();
        vk::MemoryAllocateInfo allocInfo{ .allocationSize = memRequirements.size, .memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties) };
        vk::raii::DeviceMemory bufferMemory = vk::raii::DeviceMemory(device, allocInfo);
        buffer.bindMemory(*bufferMemory, 0);
        return { std::move(buffer), std::move(bufferMemory) };
    }

    void CopyBuffer(vk::raii::CommandBuffer& commandBuffer, const vk::raii::Buffer& srcBuffer, const vk::raii::Buffer& dstBuffer, vk::DeviceSize size) {
        vk::BufferCopy region{ .srcOffset = 0,
                               .dstOffset = 0, 
                               .size = size};
        commandBuffer.copyBuffer(srcBuffer, dstBuffer, region);
    }

    vk::raii::CommandBuffer BeginSingleTimeCommands(const vk::raii::Device& device, const vk::raii::CommandPool& commandPool) {
        vk::CommandBufferAllocateInfo allocInfo{ .commandPool = commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1 };
        vk::raii::CommandBuffer       commandBuffer = std::move(vk::raii::CommandBuffers(device, allocInfo).front());

        vk::CommandBufferBeginInfo beginInfo{ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
        commandBuffer.begin(beginInfo);

        return commandBuffer;
    }

    void EndSingleTimeCommands(vk::raii::CommandBuffer&& commandBuffer, const vk::raii::Queue queue) {
        commandBuffer.end();

        vk::SubmitInfo submitInfo{ .commandBufferCount = 1, .pCommandBuffers = &*commandBuffer };
        queue.submit(submitInfo, nullptr);
        queue.waitIdle();
    }

    uint32_t FindMemoryType(vk::raii::PhysicalDevice physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
        vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

}