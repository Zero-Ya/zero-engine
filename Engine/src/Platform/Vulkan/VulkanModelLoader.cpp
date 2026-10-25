#include "VulkanModelLoader.h"

#include "ZEngine/Core/Application.h"
#include "VulkanGraphicsDevice.h"
#include "VulkanContext.h"

#include "VulkanResourceManager.h"
#include "VulkanBindlessManager.h"

#include <glm/gtc/type_ptr.hpp>

#include <stb_image.h>
#include <ktx.h>
#include <ktxvulkan.h>

namespace ZEngine {

    Scope<ModelLoader> ModelLoader::Create(
        const Scope<GraphicsDevice>& graphicsDevice,
        const Scope<ResourceManager>& resourceManager,
        const Scope<BindlessDescriptorManager>& bindlessManager
    ) {
        return std::make_unique<VulkanModelLoader>(graphicsDevice, resourceManager, bindlessManager);
    }

	VulkanModelLoader::VulkanModelLoader(
		const Scope<GraphicsDevice>& graphicsDevice,
		const Scope<ResourceManager>& resourceManager,
		const Scope<BindlessDescriptorManager>& bindlessManager
	) {
		m_GraphicsDevice = static_cast<VulkanGraphicsDevice*>(graphicsDevice.get());
		m_ResourceManager = static_cast<VulkanResourceManager*>(resourceManager.get());
		m_BindlessManager = static_cast<VulkanBindlessManager*>(bindlessManager.get());
	}

    std::optional<Model> VulkanModelLoader::LoadModel(const std::string& filePath) {
        auto vk_GraphicsDevice = static_cast<VulkanGraphicsDevice*>(Application::Get().GetGraphicsDevice().get());
        auto& vk_Context = vk_GraphicsDevice->GetContext();
        auto& device = vk_Context.GetDevice();
        auto& physicalDevice = vk_Context.GetPhysicalDevice();

        fastgltf::Parser parser(fastgltf::Extensions::KHR_materials_unlit | fastgltf::Extensions::KHR_texture_basisu);

        auto data = fastgltf::GltfDataBuffer::FromPath(filePath);
        if (data.error() != fastgltf::Error::None) {
            std::cerr << "Failed to open glTF file: " << filePath << std::endl;
            return std::nullopt;
        }

        // Determine parent directory for loading external resources (buffers/images)
        std::string pathStr = filePath;
        std::string baseDir = pathStr.substr(0, pathStr.find_last_of("/\\") + 1);

        constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember |
            fastgltf::Options::AllowDouble |
            fastgltf::Options::LoadGLBBuffers |
            fastgltf::Options::LoadExternalBuffers;

        auto asset = parser.loadGltfBinary(data.get(), baseDir, gltfOptions);
        if (asset.error() != fastgltf::Error::None) {
            // Fallback to ASCII .gltf parsing if glb parsing fails
            asset = parser.loadGltf(data.get(), baseDir, gltfOptions);
            if (asset.error() != fastgltf::Error::None) {
                std::cerr << "Failed to parse glTF asset: " << static_cast<uint64_t>(asset.error()) << std::endl;
                return std::nullopt;
            }
        }

        Model model;
        model.name = pathStr;

        // Load images
        std::vector<TextureHandle> loadedImages;
        loadedImages.reserve(asset->images.size());
        for (const auto& image : asset->images) {
            TextureHandle imageHandle = LoadGLTFTexture(baseDir, image);
            loadedImages.push_back(imageHandle);
            model.textures.push_back(imageHandle);
        }

        // Register textures into bindless manager (set 0 binding 0)
        std::vector<uint32_t> textureBindlessIndices;
        textureBindlessIndices.reserve(asset->textures.size());
        for (size_t i = 0; i < asset->textures.size(); ++i) {
            uint32_t bindlessIndex = RegisterTexture(asset.get(), i, loadedImages);
            textureBindlessIndices.push_back(bindlessIndex);
        }

        // Register sampler into bindless manager (set 0 binding 1)
        std::vector<uint32_t> samplerBindlessIndices;
        samplerBindlessIndices.reserve(asset->samplers.size());
        for (const auto& sampler : asset->samplers) {
            uint32_t bindlessIndex = RegisterSampler(sampler);
            samplerBindlessIndices.push_back(bindlessIndex);
        }

        // Load meshes and submeshes (primitives)
        model.meshes.resize(asset->meshes.size());
        for (size_t i = 0; i < asset->meshes.size(); ++i) {
            const auto& gltfMesh = asset->meshes[i];
            Mesh& engineMesh = model.meshes[i];
            engineMesh.name = std::string(gltfMesh.name);

            LoadSubmeshes(asset.get(), gltfMesh, engineMesh, textureBindlessIndices, samplerBindlessIndices);
        }

        // Build node hierarchy tree
        std::vector<std::shared_ptr<Node>> allNodes(asset->nodes.size());
        for (size_t i = 0; i < asset->nodes.size(); ++i) {
            allNodes[i] = std::make_shared<Node>();
            allNodes[i]->name = std::string(asset->nodes[i].name);
        }

        for (size_t i = 0; i < asset->nodes.size(); ++i) {
            LoadSceneNodes(asset.get(), i, allNodes);
        }

        // Collect root nodes from default scene
        const auto& scene = asset->scenes[asset->defaultScene.value_or(0)];
        for (auto nodeIdx : scene.nodeIndices) {
            model.rootNodes.push_back(allNodes[nodeIdx]);
        }

        std::cout << "[ModelLoader] Loaded hierarchical model: " << model.name
            << " (" << model.meshes.size() << " meshes, " << model.rootNodes.size() << " root nodes)" << std::endl;

        return model;
	}

    TextureHandle VulkanModelLoader::LoadGLTFTexture(const std::string& baseDir, const fastgltf::Image& image) {
        std::string texturePath;

        std::visit(fastgltf::visitor{
            [&](const fastgltf::sources::URI& filePath) {
                //auto fullPath = baseDir / filePath.uri.fspath();
                texturePath = baseDir + "/" + std::string(filePath.uri.path().begin(), filePath.uri.path().end());
            },
            [](auto&) {}
            }, image.data);

        TextureHandle imageHandle = LoadTexture(texturePath);
        return imageHandle;
    }

    uint32_t VulkanModelLoader::RegisterTexture(const fastgltf::Asset& asset, uint32_t textureIndex, std::vector<TextureHandle>& loadedImages) {
        const auto& texture = asset.textures[textureIndex];

        if (!texture.imageIndex.has_value()) return 0;
        TextureHandle imageHandle = loadedImages[texture.imageIndex.value()];

        auto bindlessIndex = m_BindlessManager->RegisterTexture(imageHandle, TextureLayout::SHADER_READ);
        return bindlessIndex;
    }

    uint32_t VulkanModelLoader::RegisterSampler(const fastgltf::Sampler& sampler) {
        vk::Filter magFilter{};
        vk::Filter minFilter{};
        vk::SamplerMipmapMode mipmapMode{};

        GetFilterModes(sampler.magFilter, magFilter, mipmapMode);
        GetFilterModes(sampler.minFilter, minFilter, mipmapMode);

        vk::SamplerAddressMode addressModeU = GetAddressMode(sampler.wrapS);
        vk::SamplerAddressMode addressModeV = GetAddressMode(sampler.wrapT);

        // Create sampler
        SamplerHandle samplerHandle = m_ResourceManager->CreateSampler({
            .magFilter = magFilter,
            .minFilter = minFilter,
            .mipmapMode = mipmapMode,
            .addressModeU = addressModeU,
            .addressModeV = addressModeV,
            .addressModeW = vk::SamplerAddressMode::eRepeat
        });

        auto bindlessIndex = m_BindlessManager->RegisterSampler(samplerHandle);
        return bindlessIndex;
    }

    void VulkanModelLoader::LoadSubmeshes(const fastgltf::Asset& asset, const fastgltf::Mesh& gltfMesh, Mesh& engineMesh, std::vector<uint32_t>& textureBindlessIndices, std::vector<uint32_t>& samplerBindlessIndices) {
        for (const auto& prim : gltfMesh.primitives) {
            Submesh submesh;

            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;

            // Extract position
            auto posIt = prim.findAttribute("POSITION");
            if (posIt == prim.attributes.end()) continue;

            const auto& posAccessor = asset.accessors[posIt->accessorIndex];
            vertices.resize(posAccessor.count);
            fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset, posAccessor, [&](fastgltf::math::fvec3 pos, size_t index) {
                vertices[index].position = glm::vec3(pos[0], pos[1], pos[2]);
                }
            );

            // Extract normal
            auto normIt = prim.findAttribute("NORMAL");
            if (normIt != prim.attributes.end()) {
                const auto& accessor = asset.accessors[normIt->accessorIndex];
                fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset, accessor, [&](fastgltf::math::fvec3 norm, size_t index) {
                    vertices[index].normal = glm::vec3(norm[0], norm[1], norm[2]);
                    }
                );
            }

            // Extract texture coords 0 (UV)
            auto uvIt = prim.findAttribute("TEXCOORD_0");
            if (uvIt != prim.attributes.end()) {
                auto& uvAccessor = asset.accessors[uvIt->accessorIndex];
                fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(asset, uvAccessor, [&](fastgltf::math::fvec2 uv, size_t index) {
                    vertices[index].uv = glm::vec2(uv[0], uv[1]);
                    }
                );
            }

            // Extract indices
            if (prim.indicesAccessor.has_value()) {
                const auto& indexAccessor = asset.accessors[prim.indicesAccessor.value()];
                indices.resize(indexAccessor.count);

                fastgltf::iterateAccessorWithIndex<uint32_t>(asset, indexAccessor, [&](uint32_t indexValue, size_t index) {
                    indices[index] = indexValue;
                    }
                );
            }

            submesh.vertexCount = static_cast<uint32_t>(vertices.size());
            submesh.indexCount = static_cast<uint32_t>(indices.size());

            // Create vertex buffer
            const uint32_t verticesSize = vertices.size() * sizeof(Vertex);
            BufferCreateInfo vertexInfo{
                .size = verticesSize,
                .usageFlags = BufferUsageFlags::VertexBuffer | BufferUsageFlags::TransferDst,
                .isHostVisible = false
            };
            submesh.vertexBuffer = m_ResourceManager->CreateBuffer(vertexInfo);

            // Create index buffer
            const uint32_t indicesSize = indices.size() * sizeof(uint32_t);
            BufferCreateInfo indexInfo{
                .size = indicesSize,
                .usageFlags = BufferUsageFlags::IndexBuffer | BufferUsageFlags::TransferDst,
                .isHostVisible = false
            };
            submesh.indexBuffer = m_ResourceManager->CreateBuffer(indexInfo);

            m_ResourceManager->UploadBufferData(submesh.vertexBuffer, vertices.data(), verticesSize, BufferUsageFlags::VertexBuffer);
            m_ResourceManager->UploadBufferData(submesh.indexBuffer, indices.data(), indicesSize, BufferUsageFlags::IndexBuffer);

            // Map material textures to bindless descriptor set and parse PBR materials
            if (prim.materialIndex.has_value()) {
                const auto& material = asset.materials[prim.materialIndex.value()];
                submesh.material.baseColorFactor = glm::make_vec4(material.pbrData.baseColorFactor.data());
                submesh.material.metallicFactor = material.pbrData.metallicFactor;
                submesh.material.roughnessFactor = material.pbrData.roughnessFactor;

                if (material.pbrData.baseColorTexture.has_value()) {
                    size_t texIndex = material.pbrData.baseColorTexture->textureIndex;
                    submesh.material.albedoTextureIndex = textureBindlessIndices[texIndex];

                    const auto& gltfTexture = asset.textures[texIndex];
                    if (gltfTexture.samplerIndex.has_value()) {
                        submesh.material.samplerIndex = samplerBindlessIndices[gltfTexture.samplerIndex.value()];
                    }
                }
                if (material.normalTexture.has_value()) {
                    size_t texIndex = material.normalTexture->textureIndex;
                    submesh.material.normalTextureIndex = textureBindlessIndices[texIndex];
                }
            }

            // Assign AABB bounds
            // Initialize bounds to extreme values
            glm::vec3 minBounds(std::numeric_limits<float>::max());
            glm::vec3 maxBounds(std::numeric_limits<float>::lowest());

            // Compute local AABB from vertices
            for (const auto& vertex : vertices) {
                minBounds = glm::min(minBounds, vertex.position);
                maxBounds = glm::max(maxBounds, vertex.position);
            }
            submesh.localAABB.min = minBounds;
            submesh.localAABB.max = maxBounds;

            

            engineMesh.primitives.push_back(submesh);
        }
    }

    void VulkanModelLoader::LoadSceneNodes(const fastgltf::Asset& asset, uint32_t nodeIndex, std::vector<std::shared_ptr<Node>> allNodes) {
        const auto& srcNode = asset.nodes[nodeIndex];
        auto& dstNode = allNodes[nodeIndex];

        if (srcNode.meshIndex.has_value()) {
            dstNode->meshIndex = static_cast<int32_t>(srcNode.meshIndex.value());
        }

        // Extract transform matrix
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

	void VulkanModelLoader::LoadMaterials(const fastgltf::Asset& asset) {
	}

    TextureHandle VulkanModelLoader::LoadTexture(const std::string& path) {
        std::filesystem::path filePath = path;
        std::string ext = filePath.extension().string().c_str();
        // Transform extension to lowercase
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".ktx" || ext == ".ktx2") {
            return LoadKTXTexture(path);
        }
        else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".bmp") {
            return LoadStandardImage(path);
        }
        throw std::runtime_error("Unsupported texture format: " + ext);
    }

    // Create standard texture image
    TextureHandle VulkanModelLoader::LoadStandardImage(const std::string& filePath) {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load((filePath).c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        vk::DeviceSize imageSize = texWidth * texHeight * 4;

        if (!pixels) throw std::runtime_error("Failed to load standard image:" + filePath);

        TextureCreateInfo createInfo {
            .width = static_cast<uint32_t>(texWidth),
            .height = static_cast<uint32_t>(texHeight),
            .format = TextureFormat::RGBA8_SRGB,
            .usageFlags = TextureUsageFlags::Sampled | TextureUsageFlags::TransferDst,
            .levelCount = 1
        };
        TextureHandle imageHandle = m_ResourceManager->CreateTexture(createInfo);

        vk::BufferImageCopy copyRegion { .bufferOffset = 0,
                                         .bufferRowLength = 0,
                                         .bufferImageHeight = 0,
                                         .imageSubresource = {.aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1},
                                         .imageOffset = {0, 0, 0},
                                         .imageExtent = {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1} };

        m_ResourceManager->UploadTextureData(imageHandle, pixels, texWidth, texHeight, imageSize, { copyRegion });

        stbi_image_free(pixels);
        return imageHandle;
    }

    // Create KTX texture image
    TextureHandle VulkanModelLoader::LoadKTXTexture(const std::string& filePath) {
        ktxTexture* kTexture = nullptr;

        KTX_error_code result = ktxTexture_CreateFromNamedFile(filePath.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &kTexture);

        if (result != KTX_SUCCESS) throw std::runtime_error("Failed to load KTX texture: " + filePath);

        // Transcode Basis Universal KTX2 textures to BC7 if needed
        if (ktxTexture_NeedsTranscoding(kTexture)) {
            ktxTexture2* kTexture2 = reinterpret_cast<ktxTexture2*>(kTexture);
            ktxTexture2_TranscodeBasis(kTexture2, KTX_TTF_BC7_RGBA, 0);
        }

        // Get format, width, height and mip levels
        vk::Format format = static_cast<vk::Format>(ktxTexture_GetVkFormat(kTexture));
        uint32_t width = kTexture->baseWidth;
        uint32_t height = kTexture->baseHeight;
        uint32_t mipLevels = kTexture->numLevels;

        TextureCreateInfo createInfo{
            .width = width,
            .height = height,
            .format = GetRHIFormat(format),
            .usageFlags = TextureUsageFlags::Sampled | TextureUsageFlags::TransferDst,
            .levelCount = mipLevels
        };
        TextureHandle imageHandle = m_ResourceManager->CreateTexture(createInfo);

        // Setup copy regions for all mip levels
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

        size_t ktxSize = ktxTexture_GetDataSize((ktxTexture*)kTexture);
        uint8_t* ktxData = ktxTexture_GetData((ktxTexture*)kTexture);
        m_ResourceManager->UploadTextureData(imageHandle, ktxData, width, height, ktxSize, copyRegions);

        ktxTexture_Destroy((ktxTexture*)kTexture);
        return imageHandle;
    }

    // Maps fastgltf filter to Vulkan vk::Filter and vk::SamplerMipmapMode
    void VulkanModelLoader::GetFilterModes(
        std::optional<fastgltf::Filter> filter,
        vk::Filter& outFilter,
        vk::SamplerMipmapMode& outMipMode)
    {
        outFilter = vk::Filter::eLinear;
        outMipMode = vk::SamplerMipmapMode::eLinear;

        if (!filter.has_value()) return;

        switch (filter.value()) {
            case fastgltf::Filter::Nearest:
                outFilter = vk::Filter::eNearest;
                outMipMode = vk::SamplerMipmapMode::eNearest;
                break;
            case fastgltf::Filter::Linear:
                outFilter = vk::Filter::eLinear;
                outMipMode = vk::SamplerMipmapMode::eLinear;
                break;
            case fastgltf::Filter::NearestMipMapNearest:
                outFilter = vk::Filter::eNearest;
                outMipMode = vk::SamplerMipmapMode::eNearest;
                break;
            case fastgltf::Filter::LinearMipMapNearest:
                outFilter = vk::Filter::eLinear;
                outMipMode = vk::SamplerMipmapMode::eNearest;
                break;
            case fastgltf::Filter::NearestMipMapLinear:
                outFilter = vk::Filter::eNearest;
                outMipMode = vk::SamplerMipmapMode::eLinear;
                break;
            case fastgltf::Filter::LinearMipMapLinear:
                outFilter = vk::Filter::eLinear;
                outMipMode = vk::SamplerMipmapMode::eLinear;
                break;
        }
    }

    // Maps fastgltf Wrap to Vulkan vk::SamplerAddressMode
    vk::SamplerAddressMode VulkanModelLoader::GetAddressMode(fastgltf::Wrap wrap) {
        switch (wrap) {
            case fastgltf::Wrap::ClampToEdge:    return vk::SamplerAddressMode::eClampToEdge;
            case fastgltf::Wrap::MirroredRepeat: return vk::SamplerAddressMode::eMirroredRepeat;
            case fastgltf::Wrap::Repeat:
            default:                             return vk::SamplerAddressMode::eRepeat;
        }
    }

    TextureFormat VulkanModelLoader::GetRHIFormat(vk::Format format) {
        switch (format) {
            case vk::Format::eUndefined:      return TextureFormat::UNDEFINED;
            case vk::Format::eR8G8B8A8Unorm:  return TextureFormat::RGBA8_UNORM;
            case vk::Format::eB8G8R8A8Unorm:  return TextureFormat::BGRA8_UNORM;
            case vk::Format::eR8G8B8A8Srgb:   return TextureFormat::RGBA8_SRGB;
            case vk::Format::eB8G8R8A8Srgb:   return TextureFormat::BGRA8_SRGB;
            case vk::Format::eD32Sfloat:      return TextureFormat::D32_FLOAT;
            case vk::Format::eD24UnormS8Uint: return TextureFormat::D24_UNORM_S8_UINT;
            case vk::Format::eBc7UnormBlock:  return TextureFormat::BC7_UNORM;
            case vk::Format::eBc7SrgbBlock:   return TextureFormat::BC7_SRBG;
            default:                          return TextureFormat::RGBA8_SRGB;
        }
    }

}