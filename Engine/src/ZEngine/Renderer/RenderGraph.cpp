#include "RenderGraph.h"

#include "ZEngine/Core/Application.h"

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

            ZEngine::RenderingInfo renderingInfo{};
            renderingInfo.renderWidth = ZEngine::Application::Get().GetWindow().GetWidth();
            renderingInfo.renderHeight = ZEngine::Application::Get().GetWindow().GetHeight();

            // Resolve and insert barrier transitions for Writes
            for (const auto& write : pass.writes) {
                uint32_t id = write.texture.id;
                TextureLayout currentLayout = TextureLayout::UNDEFINED;

                if (write.layout == TextureLayout::COLOR_ATTACHMENT) {
                    ZEngine::ColorAttachmentInfo colorAttachmentInfo {
                        .texture = write.texture,
                    };

                    if (pass.name == "OpaquePass") colorAttachmentInfo.clearOnLoad = true;
                    renderingInfo.colorAttachmentCount = 1;
                    renderingInfo.colorAttachments = std::vector(1, colorAttachmentInfo);
                }

                if (write.layout == TextureLayout::DEPTH_ATTACHMENT) {
                    ZEngine::DepthAttachmentInfo depthAttachementInfo {
                        .texture = write.texture,
                    };

                    if (pass.name == "OpaquePass" || pass.name == "ShadowPass") depthAttachementInfo.clearOnLoad = true;
                    renderingInfo.hasDepth = true;
                    renderingInfo.depthAttachment = depthAttachementInfo;
                }

                if (auto it = m_ResourceLayouts.find(id); it != m_ResourceLayouts.end()) {
                    currentLayout = it->second;
                }

                if (currentLayout != write.layout) {
                    cmdList.TransitionImageLayout(write.texture, currentLayout, write.layout);
                    m_ResourceLayouts[id] = write.layout;
                }
            }

            // Dynamic rendering
            // ...this part can be made better
            if (renderingInfo.colorAttachmentCount != 0 || renderingInfo.hasDepth)
                cmdList.BeginRendering(renderingInfo);

            cmdList.SetViewport(0, 0, ZEngine::Application::Get().GetWindow().GetWidth(), ZEngine::Application::Get().GetWindow().GetHeight());
            cmdList.SetScissor(0, 0, ZEngine::Application::Get().GetWindow().GetWidth(), ZEngine::Application::Get().GetWindow().GetHeight());
            // SetClearColor

            // Execute pass recording
            pass.execute(cmdList);

            if (renderingInfo.colorAttachmentCount != 0 || renderingInfo.hasDepth)
                cmdList.EndRendering();
        }
    }

    void RenderGraph::Clear() {
        m_Passes.clear();
        m_ResourceLayouts.clear();
    }

}