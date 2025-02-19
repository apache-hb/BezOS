#pragma once

#include "memory/range.hpp"
#include "util/memory.hpp"

namespace km {
    static constexpr size_t kLowMemory = sm::megabytes(1).bytes();

    constexpr bool IsLowMemory(km::MemoryRange range) {
        return range.front < kLowMemory;
    }
}
