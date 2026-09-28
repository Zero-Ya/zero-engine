#pragma once

namespace ZEngine {

    class PipelineState;

    class Model {
    public:
        virtual ~Model() = default;

        virtual bool LoadFromFile(const std::string& filePath) = 0;
        virtual void Draw(const Ref<PipelineState>& pipelineState) = 0;

        static Scope<Model> Create();
    };

}