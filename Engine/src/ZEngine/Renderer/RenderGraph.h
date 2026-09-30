#pragma once

#include "ZEngine/RHI/RHICommon.h"
#include "ZEngine/RHI/RHICommandList.h"

namespace ZEngine {

    struct PassResourceAccess {
        TextureHandle texture;
        TextureLayout layout;
    };

    class RenderGraphPassBuilder {
    public:
        explicit RenderGraphPassBuilder(const std::string& name) : m_Name(name) {}

        // Declare layout needed by this pass
        RenderGraphPassBuilder& Read(TextureHandle texture, TextureLayout layout = TextureLayout::SHADER_READ) {
            m_Reads.push_back({ texture, layout });
            return *this;
        }

        RenderGraphPassBuilder& WriteColor(TextureHandle texture) {
            m_Writes.push_back({ texture, TextureLayout::COLOR_ATTACHMENT });
            return *this;
        }

        RenderGraphPassBuilder& WriteDepth(TextureHandle texture) {
            m_Writes.push_back({ texture, TextureLayout::DEPTH_ATTACHMENT });
            return *this;
        }

        const std::string& GetName() const { return m_Name; }
        const std::vector<PassResourceAccess>& GetReads() const { return m_Reads; }
        const std::vector<PassResourceAccess>& GetWrites() const { return m_Writes; }

    private:
        std::string m_Name;
        std::vector<PassResourceAccess> m_Reads;
        std::vector<PassResourceAccess> m_Writes;
    };

    using PassExecuteCallback = std::function<void(RHICommandList&)>;

    struct RenderGraphPassNode {
        std::string name;
        std::vector<PassResourceAccess> reads;
        std::vector<PassResourceAccess> writes;
        PassExecuteCallback execute;
    };

    class RenderGraph {
    public:
        RenderGraph() = default;
        ~RenderGraph() = default;

        // Add pass with builder-lambda setup pattern
        template <typename ExecuteFn>
        void AddPass(const std::string& name, std::function<void(RenderGraphPassBuilder&)> setup, ExecuteFn&& execute) {
            RenderGraphPassBuilder builder(name);
            setup(builder);

            RenderGraphPassNode node{};
            node.name = builder.GetName();
            node.reads = builder.GetReads();
            node.writes = builder.GetWrites();
            node.execute = std::forward<ExecuteFn>(execute);

            m_Passes.push_back(std::move(node));
        }

        // Compile and execute recorded graph into RHICommandList
        void Execute(RHICommandList& cmdList);

        // Reset graph frame data
        void Clear();

    private:
        std::vector<RenderGraphPassNode> m_Passes;

        // Track layout state across pass transitions
        std::unordered_map<uint32_t, TextureLayout> m_ResourceLayouts;
    };

}