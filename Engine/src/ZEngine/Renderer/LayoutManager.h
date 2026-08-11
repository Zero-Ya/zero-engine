#pragma once

namespace ZEngine {

    enum class SetSlot : uint32_t {
        Global = 0,
        Pass = 1,
        Material = 2,
        Object = 3,
    };

    class LayoutManager {
    public:
        virtual ~LayoutManager() = default;

        virtual void Init() = 0;

        static Scope<LayoutManager> Create();
    };

}