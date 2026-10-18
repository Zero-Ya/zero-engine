#pragma once

#include "ZEngine/RHI/RHICommon.h"

#include <queue>

namespace ZEngine {

    template<typename HandleType, typename ResourceType>
    class GenerationalPool {
    public:
        HandleType Allocate(ResourceType resource) {
            uint32_t index = 0;

            if (!m_FreeIndices.empty()) {
                index = m_FreeIndices.front();
                m_FreeIndices.pop();
            }
            else {
                index = static_cast<uint32_t>(m_Slots.size());
                m_Slots.emplace_back();
            }

            PoolSlot& slot = m_Slots[index];
            slot.resource = std::move(resource);
            slot.isAlive = true;

            return HandleType::Make(index, slot.generation);
        }

        bool Free(HandleType handle) {
            if (!IsValid(handle)) return false;

            uint32_t index = handle.GetIndex();
            PoolSlot& slot = m_Slots[index];

            slot.resource = ResourceType {}; // Reset RAII handles
            slot.isAlive = false;
            slot.generation = (slot.generation + 1) & HANDLE_GEN_MASK; // Increment generation

            m_FreeIndices.push(index);
            return true;
        }

        ResourceType* Get(HandleType handle) {
            if (!IsValid(handle)) return nullptr;
            return &m_Slots[handle.GetIndex()].resource;
        }

        const ResourceType* Get(HandleType handle) const {
            if (!IsValid(handle)) return nullptr;
            return &m_Slots[handle.GetIndex()].resource;
        }

        bool IsValid(HandleType handle) const {
            if (!handle.IsValid()) return false;

            uint32_t index = handle.GetIndex();
            if (index >= m_Slots.size()) return false;

            const PoolSlot& slot = m_Slots[index];
            return slot.isAlive && (slot.generation == handle.GetGeneration());
        }

        void Clear() {
            m_Slots.clear();
            std::queue<uint32_t> emptyQueue;
            std::swap(m_FreeIndices, emptyQueue);
        }

    private:
        struct PoolSlot {
            ResourceType resource{};
            uint32_t generation = 0;
            bool isAlive = false;
        };

    private:
        std::vector<PoolSlot> m_Slots;
        std::queue<uint32_t> m_FreeIndices;
    };

}