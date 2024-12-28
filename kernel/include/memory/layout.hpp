#pragma once

#include "kernel.hpp"

#include "memory/memory.hpp"

#include "std/static_vector.hpp"

namespace km {
    struct SystemMemoryLayout {
        using FreeMemoryRanges = stdx::StaticVector<MemoryRange, 32>;
        using ReservedMemoryRanges = stdx::StaticVector<MemoryRange, 32>;

        FreeMemoryRanges available;
        FreeMemoryRanges reclaimable;
        ReservedMemoryRanges reserved;

        void reclaimBootMemory();

        static SystemMemoryLayout from(KernelMemoryMap memmap);
    };
}
