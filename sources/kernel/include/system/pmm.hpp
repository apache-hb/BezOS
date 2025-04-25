#pragma once

#include "memory/heap.hpp"
#include "memory/range.hpp"
#include "util/absl.hpp"

#include <atomic>

namespace sys2 {
    struct MemorySegment {
        std::atomic<uint8_t> mOwners;
        km::TlsfAllocation mHandle;

        MemorySegment(km::TlsfAllocation handle)
            : MemorySegment(1, handle)
        { }

        MemorySegment(uint8_t owners, km::TlsfAllocation handle)
            : mOwners(owners)
            , mHandle(handle)
        { }

        MemorySegment(MemorySegment&& other)
            : mOwners(other.mOwners.load())
            , mHandle(std::move(other.mHandle))
        { }
    };

    struct MemoryManagerStats {
        km::TlsfHeapStats heapStats;
        size_t segments;
    };

    class MemoryManager {
        using Counter = std::atomic<uint8_t>;
        using Map = sm::BTreeMap<km::PhysicalAddress, MemorySegment>;
        using Iterator = typename Map::iterator;

        km::MemoryRange mRange;
        Map mSegments;
        km::TlsfHeap mHeap;

        MemoryManager(km::MemoryRange range, km::TlsfHeap&& heap)
            : mRange(range)
            , mHeap(std::move(heap))
        { }

        bool releaseSegment(Iterator it);
    public:
        MemoryManager() = default;

        OsStatus retain(km::MemoryRange range) [[clang::allocating]];
        OsStatus release(km::MemoryRange range) [[clang::allocating]];
        OsStatus allocate(size_t size, km::MemoryRange *range) [[clang::allocating]];

        [[nodiscard]]
        static OsStatus create(km::MemoryRange range, MemoryManager *manager) [[clang::allocating]];

        [[nodiscard]]
        MemoryManagerStats stats() noexcept [[clang::nonallocating]];
    };
}
