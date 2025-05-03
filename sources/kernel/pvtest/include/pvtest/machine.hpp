#pragma once

#include "absl/container/fixed_array.h"

#include "pvtest/memory.hpp"
#include "pvtest/cpu.hpp"
#include "util/memory.hpp"

namespace pv {
    class CpuCore;

    class Machine {
        absl::FixedArray<CpuCore, absl::kFixedArrayUseDefault, SharedAllocator<CpuCore>> mCores;
        Memory mMemory;
    public:
        Machine(size_t cores, off64_t memorySize = sm::gigabytes(16).bytes());

        PVTEST_SHARED_OBJECT(Machine);

        static km::VirtualRangeEx getSharedMemory();

        static void init();
        static void finalize();

        static void initChild();
        static void finalizeChild();
    };
}
