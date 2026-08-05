#pragma once

namespace ZEngine {

    class LayoutManager {
    public:
        virtual ~LayoutManager() = default;

        virtual void Init() = 0;

        static std::unique_ptr<LayoutManager> Create();
    };

}