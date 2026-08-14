#pragma once

#include <glm/glm.hpp>

namespace ZEngine {

    enum class SetSlot : uint32_t {
        Global = 0,
        Pass = 1,
        Material = 2,
    };

    struct PushConstantData {
        glm::mat4 transform { 1.0f };
    };

    class LayoutManager {
    public:
        virtual ~LayoutManager() = default;

        virtual void Init() = 0;

        static Scope<LayoutManager> Create();
    };

}