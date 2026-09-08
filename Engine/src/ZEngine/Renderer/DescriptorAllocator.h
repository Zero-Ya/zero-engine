#pragma once

namespace ZEngine {

    class DescriptorAllocator {
    public:
        virtual ~DescriptorAllocator() = default;

        virtual void Clear() = 0;

        static Scope<DescriptorAllocator> Create();
    };

}