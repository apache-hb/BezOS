#pragma once

#include "memory/range.hpp"

#include <atomic>
#include <memory>

namespace sys2 {
    class MemoryManager {
        using Counter = std::atomic<uint8_t>;

        std::unique_ptr<Counter[]> mPageUsage;
        km::MemoryRange mRange;

        MemoryManager(km::MemoryRange range);
    public:
        MemoryManager() = default;

        OsStatus retain(km::MemoryRange range);
        OsStatus release(km::MemoryRange range);

        [[nodiscard]]
        static OsStatus create(km::MemoryRange range, MemoryManager *manager) [[clang::allocating]];
    };
}
