#pragma once

namespace ZEngine {

    class RenderCommandBuffer;
    class PipelineState;

    class Model {
    public:
        virtual ~Model() = default;

        virtual bool LoadFromFile(const std::string& filePath) = 0;
        virtual void Draw(const Ref<RenderCommandBuffer>& commandBuffer, const Ref<PipelineState>& pipelineState) = 0;

        static Scope<Model> Create();
    };

}