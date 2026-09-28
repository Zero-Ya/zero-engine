#pragma once

namespace ZEngine {

	// Opaque handle types
    struct BufferHandle {
        uint64_t id = 0;
        bool IsValid() const { return id != 0; }
        bool operator==(const BufferHandle& other) const { return id == other.id; }
        bool operator!=(const BufferHandle& other) const { return id != other.id; }
    };

    struct TextureHandle {
        uint64_t id = 0;
        bool IsValid() const { return id != 0; }
        bool operator==(const TextureHandle& other) const { return id == other.id; }
        bool operator!=(const TextureHandle& other) const { return id != other.id; }
    };

    // Reserved Pseudo-Handles for Swapchain Targets
    inline constexpr TextureHandle SWAPCHAIN_TEXTURE_HANDLE{ 0xFFFFFFFFFFFFFFFF };
    inline constexpr TextureHandle SWAPCHAIN_DEPTH_HANDLE{ 0xFFFFFFFFFFFFFFFE };

    struct PipelineHandle {
        uint64_t id = 0;
        bool IsValid() const { return id != 0; }
        bool operator==(const TextureHandle& other) const { return id == other.id; }
        bool operator!=(const TextureHandle& other) const { return id != other.id; }
    };

    // RHI Enums

    enum class BufferUsageFlags : uint32_t {
        VertexBuffer = 1 << 0,
        IndexBuffer = 1 << 1,
        UniformBuffer = 1 << 2,
        StorageBuffer = 1 << 3,
        TransferSrc = 1 << 4,
        TransferDst = 1 << 5
    };

    inline BufferUsageFlags operator|(BufferUsageFlags a, BufferUsageFlags b) {
        return static_cast<BufferUsageFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    struct BufferCreateInfo {
        uint64_t size = 0;
        BufferUsageFlags usageFlags;
        bool isHostVisible = false; // Persistent mapping flag
    };

    enum class TextureFormat {
        UNDEFINED,
        RGBA8_UNORM,
        BGRA8_UNORM,
        RGBA8_SRGB,
        BGRA8_SRGB,
        D32_FLOAT,
        D24_UNORM_S8_UINT
    };

    enum class TextureUsageFlags : uint32_t {
        Sampled = 1 << 0,
        ColorAttachment = 1 << 1,
        DepthAttachment = 1 << 2,
        TransferSrc = 1 << 3,
        TransferDst = 1 << 4
    };

    struct TextureCreateInfo {
        uint32_t width = 0;
        uint32_t height = 0;
        TextureFormat format = TextureFormat::RGBA8_UNORM;
        TextureUsageFlags usageFlags;
    };

    enum class TextureLayout {
        UNDEFINED,
        GENERAL,
        COLOR_ATTACHMENT,
        DEPTH_ATTACHMENT,
        SHADER_READ,
        TRANSFER_SRC,
        TRANSFER_DST,
        PRESENT_SRC
    };

    enum class IndexType {
        UINT16,
        UINT32
    };

    enum class ShaderStageFlags : uint32_t {
        None     = 0,
        Vertex   = 1 << 0,
        Fragment = 1 << 1,
        Compute  = 1 << 2,
        All      = Vertex | Fragment | Compute
    };

    inline ShaderStageFlags operator|(ShaderStageFlags a, ShaderStageFlags b) {
        return static_cast<ShaderStageFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    // Attachment infos for dynamic rendering
    struct ClearColorValue {
        float r = 0.0f;
        float g = 0.0f;
        float b = 0.0f;
        float a = 1.0f;

        //float colorValue[4]{ 0.0f, 0.0f, 0.0f, 1.0f };
    };

    struct ColorAttachmentInfo {
        TextureHandle texture;
        TextureLayout layout = TextureLayout::COLOR_ATTACHMENT;
        //std::array<float, 4> clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
        ClearColorValue clearColor { 0.0f, 0.0f, 0.0f, 1.0f };
        bool clearOnLoad = true;
    };

    struct DepthAttachmentInfo {
        TextureHandle texture;
        TextureLayout layout = TextureLayout::DEPTH_ATTACHMENT;
        float clearDepth = 1.0f;
        uint32_t clearStencil = 0;
        bool clearOnLoad = true;
    };

    struct RenderingInfo {
        uint32_t renderWidth = 0;
        uint32_t renderHeight = 0;
        std::vector<ColorAttachmentInfo> colorAttachments;
        uint32_t colorAttachmentCount = 0;
        DepthAttachmentInfo depthAttachment{};
        bool hasDepth = false;
    };

}