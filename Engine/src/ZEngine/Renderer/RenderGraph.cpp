#include "RenderGraph.h"

namespace ZEngine {

    void RenderGraph::Execute(RHICommandList& cmdList) {
        for (const auto& pass : m_Passes) {
            // Resolve and insert barrier transitions for Reads
            for (const auto& read : pass.reads) {
                uint32_t id = read.texture.id;
                TextureLayout currentLayout = TextureLayout::UNDEFINED;

                if (auto it = m_ResourceLayouts.find(id); it != m_ResourceLayouts.end()) {
                    currentLayout = it->second;
                }

                if (currentLayout != read.layout) {
                    cmdList.TransitionImageLayout(read.texture, currentLayout, read.layout);
                    m_ResourceLayouts[id] = read.layout;
                }
            }

            // Resolve and insert barrier transitions for Writes
            for (const auto& write : pass.writes) {
                uint32_t id = write.texture.id;
                TextureLayout currentLayout = TextureLayout::UNDEFINED;

                if (auto it = m_ResourceLayouts.find(id); it != m_ResourceLayouts.end()) {
                    currentLayout = it->second;
                }

                if (currentLayout != write.layout) {
                    cmdList.TransitionImageLayout(write.texture, currentLayout, write.layout);
                    m_ResourceLayouts[id] = write.layout;
                }
            }

            // Execute pass recording
            pass.execute(cmdList);
        }
    }

    void RenderGraph::Clear() {
        m_Passes.clear();
        m_ResourceLayouts.clear();
    }

}