#include "VulkanModel.h"

#include "ZEngine/Core/Application.h"
#include "VulkanContext.h"

#include "VulkanBuffer.h"
#include "VulkanMaterial.h"
#include "VulkanCommandBuffer.h"
#include "VulkanPipelineState.h"
#include "Platform/Vulkan/VulkanDescriptorAllocator.h"

namespace {

    std::pair<vk::raii::Image, vk::raii::DeviceMemory> CreateImage(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, uint32_t width, uint32_t height, vk::Format format, uint32_t mipLevels, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties);
    std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> CreateBuffer(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
    uint32_t FindMemoryType(vk::raii::PhysicalDevice physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties);
    vk::raii::ImageView CreateImageView(const vk::raii::Device& device, vk::Image const& image, vk::Format format, uint32_t mipLevels);
    vk::raii::Sampler CreateSampler(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, float maxLod);
    void TransitionImageLayout(vk::raii::CommandBuffer& commandBuffer, const vk::raii::Image& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
    void CopyBufferToImage(vk::raii::CommandBuffer& commandBuffer, const vk::raii::Buffer& buffer, vk::raii::Image& image, uint32_t width, uint32_t height, const std::vector<vk::BufferImageCopy>& regions);
    //void CopyBufferToImage(vk::raii::CommandBuffer& commandBuffer, const vk::raii::Buffer& buffer, vk::raii::Image& image, uint32_t width, uint32_t height);
    void CopyBuffer(vk::raii::CommandBuffer& commandBuffer, const vk::raii::Buffer& srcBuffer, const vk::raii::Buffer& dstBuffer, vk::DeviceSize size);

    vk::raii::CommandBuffer BeginSingleTimeCommands(const vk::raii::Device& device, const vk::raii::CommandPool& commandPool);
    void EndSingleTimeCommands(vk::raii::CommandBuffer&& commandBuffer, const vk::raii::Queue queue);

}

namespace ZEngine {

    ModelTexture2D VulkanModel::LoadKTXTexture(const std::string& filePath) {
        auto vk_Context = static_cast<VulkanContext*>(Application::Get().GetGraphicsContext());
        auto& device = vk_Context->GetDevice();
        auto& physicalDevice = vk_Context->GetPhysicalDevice();

        ModelTexture2D texture{};
        //ktxTexture2* kTexture = nullptr;
        ktxTexture* kTexture = nullptr;

        //KTX_error_code result = ktxTexture2_CreateFromNamedFile(filePath.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &kTexture);

        KTX_error_code result = ktxTexture_CreateFromNamedFile(filePath.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &kTexture);

        if (result != KTX_SUCCESS) {
            throw std::runtime_error("Failed to load KTX texture: " + filePath);
        }

        // Transcode KTX2 if compressed with Basis Universal
        if (ktxTexture_NeedsTranscoding(kTexture)) {
            ktxTexture2* kTexture2 = reinterpret_cast<ktxTexture2*>(kTexture);
            ktxTexture2_TranscodeBasis(kTexture2, KTX_TTF_BC7_RGBA, 0);
        }

        // Transcode if Basis Universal supercompressed
        //if (ktxTexture2_NeedsTranscoding(kTexture)) {
        //    ktxTexture2_TranscodeBasis(kTexture, KTX_TTF_BC7_RGBA, 0); // Target BC7 for Desktop
        //}

        // Allocate VkImage and VkDeviceMemory using libktx Vulkan helper or manual staging
        // (Manual staging pattern shown below for clarity)

        //vk::Format format = static_cast<vk::Format>(kTexture->vkFormat);
        vk::Format format = static_cast<vk::Format>(ktxTexture_GetVkFormat(kTexture));
        uint32_t width = kTexture->baseWidth;
        uint32_t height = kTexture->baseHeight;
        uint32_t mipLevels = kTexture->numLevels;

        // Create image and allocate/bind memory (NEW)
        std::tie(texture.image, texture.memory) = CreateImage(device, physicalDevice, width, height,
            format,
            mipLevels,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
            vk::MemoryPropertyFlagBits::eDeviceLocal);


        // Uploading data via staging buffer (NEW)
        size_t ktxSize = ktxTexture_GetDataSize((ktxTexture*)kTexture);
        uint8_t* ktxData = ktxTexture_GetData((ktxTexture*)kTexture);
        auto [stagingBuffer, stagingBufferMemory] = CreateBuffer(device, physicalDevice, ktxSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        // Write to buffer
        void* data = stagingBufferMemory.mapMemory(0, ktxSize);
        memcpy(data, ktxData, ktxSize);
        stagingBufferMemory.unmapMemory();

        // 4. Setup Copy Regions for All Mip Levels
        std::vector<vk::BufferImageCopy> copyRegions;
        for (uint32_t level = 0; level < mipLevels; ++level) {
            ktx_size_t offset = 0;
            ktxTexture_GetImageOffset((ktxTexture*)kTexture, level, 0, 0, &offset);

            vk::BufferImageCopy region{};
            region.bufferOffset = offset;
            region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            region.imageSubresource.mipLevel = level;
            region.imageSubresource.layerCount = 1;
            region.imageExtent = vk::Extent3D{
                std::max(1u, width >> level),
                std::max(1u, height >> level),
                1
            };
            copyRegions.push_back(region);
        }

        // Execute copy command (Transition UNDEFINED -> TRANSFER_DST -> SHADER_READ_ONLY)
        // ... [Execute command buffer containing vkCmdCopyBufferToImage] ...
        auto& commandPool = vk_Context->GetCommandPool();
        auto queue = vk_Context->GetGraphicsQueue();

        vk::raii::CommandBuffer commandBuffer = BeginSingleTimeCommands(device, commandPool);
        TransitionImageLayout(commandBuffer, texture.image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
        CopyBufferToImage(commandBuffer, stagingBuffer, texture.image, static_cast<uint32_t>(width), static_cast<uint32_t>(height), copyRegions);
        TransitionImageLayout(commandBuffer, texture.image, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
        EndSingleTimeCommands(std::move(commandBuffer), queue);

        ktxTexture_Destroy((ktxTexture*)kTexture);

        // Create image view and sampler (NEW)
        texture.imageView = CreateImageView(device, texture.image, format, mipLevels);
        texture.sampler = CreateSampler(device, physicalDevice, static_cast<float>(mipLevels));

        // Allocate and update descriptor Set
        //auto vk_Allocator = static_cast<VulkanDescriptorAllocator*>(vk_Context->GetDescriptorAllocator().get());
        //texture.descriptorSet = vk_Allocator->Allocate(SetSlot::Material);

        //vk::DescriptorImageInfo imageInfo { .sampler = texture.sampler, .imageView = texture.imageView, .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal };
        //vk::WriteDescriptorSet descriptorWrite { .dstSet = texture.descriptorSet,
        //                                         .dstBinding = 1, // Set 2, Binding 1 (Sampler)
        //                                         .dstArrayElement = 0,
        //                                         .descriptorCount = 1,
        //                                         .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        //                                         .pImageInfo = &imageInfo };

        //device.updateDescriptorSets(descriptorWrite, {});

        return texture;
    }

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

        // 5. Parse Scene Node Hierarchy
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
        // 1. Process mesh primitives attached to this node
        if (node.meshIndex != -1) {
            // Compute and push the accumulated global matrix (World Transform)
            glm::mat4 globalMatrix = node.GetGlobalMatrix();
            cmd.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(glm::mat4), glm::value_ptr(globalMatrix));

            const Mesh& mesh = m_Meshes[node.meshIndex];

            for (const Primitive& prim : mesh.primitives) {
                // 2. Bind material/texture descriptor set (Set 1) if assigned
                if (prim.materialIndex >= 0 && prim.materialIndex < m_Textures.size()) {
                    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, static_cast<uint32_t>(SetSlot::Material), *m_Materials[prim.materialIndex].descriptorSet, nullptr);
                }

                // 3. Issue indexed draw call using submesh offsets
                cmd.drawIndexed(prim.indexCount, 1, prim.firstIndex, 0, 0);
            }
        }

        // 3. Recurse down to child nodes
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

        // 1. Bind global continuous vertex buffer once for all nodes
        vk::Buffer vertexBuffers[] = { *rawVertexBuffer };
        vk::DeviceSize offsets[] = { 0 };
        cmd.bindVertexBuffers(0, vertexBuffers, offsets);

        // 2. Bind global index buffer once
        cmd.bindIndexBuffer(*rawIndexBuffer, 0, vk::IndexType::eUint32);

        // 3. Traverse all root nodes in the scene hierarchy
        for (const auto& rootNode : m_RootNodes) {
            DrawNode(cmd, pipelineLayout, *rootNode);
        }
    }

    void VulkanModel::LoadTexture(const std::string& baseDir) {
        // ------------------------------------------------------------------------
        // STEP A: Load Textures
        // ------------------------------------------------------------------------
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
                    // If embedded, you can load from memory buffer directly
                },
                [](const auto&) {}
                }, gltfImage.data);

            // Call KTX Texture Loader
            ModelTexture2D tex = LoadKTXTexture(texturePath);
            m_Textures.push_back(std::move(tex));
        }
    }

    void VulkanModel::LoadMaterial() {
        auto vk_Context = static_cast<VulkanContext*>(Application::Get().GetGraphicsContext());
        auto& device = vk_Context->GetDevice();
        auto& physicalDevice = vk_Context->GetPhysicalDevice();

        // ------------------------------------------------------------------------
        // STEP B: Build Materials & Allocate Descriptor Sets (Set Slot 2)
        // ------------------------------------------------------------------------
        m_Materials.reserve(m_Asset.materials.size());

        for (const auto& gltfMaterial : m_Asset.materials) {
            ModelMaterial mat{};

            // 1. Extract PBR Material Parameters
            MaterialProperties params{};
            params.Albedo = glm::make_vec4(gltfMaterial.pbrData.baseColorFactor.data());
            params.Metallic = gltfMaterial.pbrData.metallicFactor;
            params.Roughness = gltfMaterial.pbrData.roughnessFactor;

            // 2. Create Material UBO
            vk::BufferCreateInfo uboInfo{};
            uboInfo.size = sizeof(MaterialProperties);
            uboInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
            mat.ubo = vk::raii::Buffer(device, uboInfo);

            vk::MemoryRequirements memRequirements = mat.ubo.getMemoryRequirements();
            vk::MemoryAllocateInfo allocInfo{ .allocationSize = memRequirements.size, .memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent) };
            mat.uboMemory = vk::raii::DeviceMemory(device, allocInfo);
            mat.ubo.bindMemory(*mat.uboMemory, 0);

            // Copy material factors into UBO
            void* data = mat.uboMemory.mapMemory(0, sizeof(MaterialProperties));
            memcpy(data, &params, static_cast<size_t>(sizeof(MaterialProperties)));
            mat.uboMemory.unmapMemory();

            // 3. Allocate Descriptor Set 2
            auto vk_Allocator = static_cast<VulkanDescriptorAllocator*>(vk_Context->GetDescriptorAllocator().get());
            mat.descriptorSet = vk_Allocator->Allocate(SetSlot::Material);

            // 4. Determine Albedo Texture Index
            uint32_t textureIndex = 0; // Default to first loaded texture
            if (gltfMaterial.pbrData.baseColorTexture.has_value()) {
                textureIndex = static_cast<uint32_t>(gltfMaterial.pbrData.baseColorTexture->textureIndex);
            }

            const ModelTexture2D& targetTex = m_Textures[textureIndex];

            // 5. Update Descriptor Writes
            vk::DescriptorBufferInfo bufferInfo{ mat.ubo, 0, sizeof(MaterialProperties) };
            vk::DescriptorImageInfo imageInfo{ targetTex.sampler, targetTex.imageView, vk::ImageLayout::eShaderReadOnlyOptimal };

            std::array<vk::WriteDescriptorSet, 2> descriptorWrites{};

            // Binding 0: Material UBO
            descriptorWrites[0].dstSet = mat.descriptorSet;
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            // Binding 1: Combined Image Sampler
            descriptorWrites[1].dstSet = mat.descriptorSet;
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
            descriptorWrites[1].pImageInfo = &imageInfo;

            device.updateDescriptorSets(descriptorWrites, nullptr);

            m_Materials.push_back(std::move(mat));
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

                // -------------------------------------------------------------------------
                // A. Extract POSITION
                // -------------------------------------------------------------------------
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

                // -------------------------------------------------------------------------
                // B. Extract NORMAL (Optional fallback if missing)
                // -------------------------------------------------------------------------
                auto normIt = prim.findAttribute("NORMAL");
                if (normIt != prim.attributes.end()) {
                    auto& normAccessor = m_Asset.accessors[normIt->accessorIndex];
                    fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(m_Asset, normAccessor, [&](fastgltf::math::fvec3 element, size_t index) {
                        allVertices[vertexStart + index].normal = glm::vec3(element[0], element[1], element[2]);
                        }
                    );
                }

                // -------------------------------------------------------------------------
                // C. Extract TEXCOORD_0 (Optional fallback if missing)
                // -------------------------------------------------------------------------
                auto uvIt = prim.findAttribute("TEXCOORD_0");
                if (uvIt != prim.attributes.end()) {
                    auto& uvAccessor = m_Asset.accessors[uvIt->accessorIndex];
                    fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(m_Asset, uvAccessor, [&](fastgltf::math::fvec2 element, size_t index) {
                        allVertices[vertexStart + index].uv = glm::vec2(element[0], element[1]);
                        }
                    );
                }

                // -------------------------------------------------------------------------
                // D. Extract INDICES
                // -------------------------------------------------------------------------
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

    std::pair<vk::raii::Image, vk::raii::DeviceMemory> CreateImage(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, uint32_t width, uint32_t height, vk::Format format, uint32_t mipLevels, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties) {
        vk::ImageCreateInfo imageInfo{ .imageType = vk::ImageType::e2D,
                                       .format = format,
                                       .extent = {width, height, 1},
                                       .mipLevels = mipLevels,
                                       .arrayLayers = 1,
                                       .samples = vk::SampleCountFlagBits::e1,
                                       .tiling = tiling,
                                       .usage = usage,
                                       .sharingMode = vk::SharingMode::eExclusive };

        vk::raii::Image image = vk::raii::Image(device, imageInfo);

        vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
        vk::MemoryAllocateInfo allocInfo{ .allocationSize = memRequirements.size,
                                         .memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties) };
        vk::raii::DeviceMemory imageMemory = vk::raii::DeviceMemory(device, allocInfo);
        image.bindMemory(imageMemory, 0);

        return { std::move(image), std::move(imageMemory) };
    }

    std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> CreateBuffer(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties) {
        vk::BufferCreateInfo   bufferInfo{ .size = size, .usage = usage, .sharingMode = vk::SharingMode::eExclusive };
        vk::raii::Buffer       buffer = vk::raii::Buffer(device, bufferInfo);
        vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();
        vk::MemoryAllocateInfo allocInfo{ .allocationSize = memRequirements.size, .memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties) };
        vk::raii::DeviceMemory bufferMemory = vk::raii::DeviceMemory(device, allocInfo);
        buffer.bindMemory(*bufferMemory, 0);
        return { std::move(buffer), std::move(bufferMemory) };
    }

    vk::raii::ImageView CreateImageView(const vk::raii::Device& device, vk::Image const& image, vk::Format format, uint32_t mipLevels) {
        vk::ImageViewCreateInfo viewInfo{
            .image = image,
            .viewType = vk::ImageViewType::e2D,
            .format = format,
            .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .baseMipLevel = 0, .levelCount = mipLevels, .baseArrayLayer = 0, .layerCount = 1} };
        return vk::raii::ImageView(device, viewInfo);
    }

    vk::raii::Sampler CreateSampler(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, float maxLod) {
        vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();
        vk::SamplerCreateInfo        samplerInfo{ .magFilter = vk::Filter::eLinear,
                                                 .minFilter = vk::Filter::eLinear,
                                                 .mipmapMode = vk::SamplerMipmapMode::eLinear,
                                                 .addressModeU = vk::SamplerAddressMode::eRepeat,
                                                 .addressModeV = vk::SamplerAddressMode::eRepeat,
                                                 .addressModeW = vk::SamplerAddressMode::eRepeat,
                                                 .mipLodBias = 0.0f,
                                                 .anisotropyEnable = vk::False,
                                                 .maxAnisotropy = 0.0f,
                                                 .compareEnable = vk::False,
                                                 .compareOp = {},
                                                 .minLod = 0.0f,
                                                 .maxLod = maxLod };
        return vk::raii::Sampler(device, samplerInfo);
    }

    void TransitionImageLayout(vk::raii::CommandBuffer& commandBuffer, const vk::raii::Image& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout) {
        vk::ImageMemoryBarrier barrier{ .oldLayout = oldLayout,
                                       .newLayout = newLayout,
                                       .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
                                       .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
                                       .image = image,
                                       .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .levelCount = 1, .layerCount = 1} };

        vk::PipelineStageFlags sourceStage;
        vk::PipelineStageFlags destinationStage;

        if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
        {
            barrier.srcAccessMask = {};
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

            sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
            destinationStage = vk::PipelineStageFlagBits::eTransfer;
        }
        else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
        {
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

            sourceStage = vk::PipelineStageFlagBits::eTransfer;
            destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
        }
        else
        {
            throw std::invalid_argument("unsupported layout transition!");
        }
        commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, {}, {}, barrier);
    }

    void CopyBufferToImage(vk::raii::CommandBuffer& commandBuffer, const vk::raii::Buffer& buffer, vk::raii::Image& image, uint32_t width, uint32_t height, const std::vector<vk::BufferImageCopy>& regions) {
        vk::BufferImageCopy region{ .bufferOffset = 0,
                                    .bufferRowLength = 0,
                                    .bufferImageHeight = 0,
                                    .imageSubresource = {.aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1},
                                    .imageOffset = {0, 0, 0},
                                    .imageExtent = {width, height, 1} };
        commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, regions);
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