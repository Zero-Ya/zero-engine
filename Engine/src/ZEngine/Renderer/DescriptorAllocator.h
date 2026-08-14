#pragma once

namespace ZEngine {

    class LayoutManager;

    class DescriptorAllocator {
    public:
        virtual ~DescriptorAllocator() = default;

        virtual void Clear() = 0;

        static Scope<DescriptorAllocator> Create(const Scope <LayoutManager>& layoutManager);
    };

}