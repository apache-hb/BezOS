#pragma once

#include "memory/heap.hpp"
#include "memory/range.hpp"
#include "util/absl.hpp"

#include <atomic>
#include <memory>

namespace sys2 {
    class MemorySegment {
        std::atomic<uint8_t> mOwners;
        km::MemoryRange mRange;
        km::TlsfAllocation mHandle;
    };

    class MemoryManager {
        using Counter = std::atomic<uint8_t>;

        std::unique_ptr<Counter[]> mPageUsage;
        km::MemoryRange mRange;
        sm::BTreeMap<km::PhysicalAddress, MemorySegment> mSegments;

        MemoryManager(km::MemoryRange range);
    public:
        MemoryManager() = default;

        OsStatus retain(km::MemoryRange range);
        OsStatus release(km::MemoryRange range);

        [[nodiscard]]
        static OsStatus create(km::MemoryRange range, MemoryManager *manager) [[clang::allocating]];
    };
}
