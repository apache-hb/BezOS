#pragma once

#include "absl/container/fixed_array.h"

#include "pvtest/cpu.hpp"
#include "pvtest/memory.hpp"
#include "util/memory.hpp"

namespace pv {
    class Machine {
        absl::FixedArray<CpuCore> mCores;
        Memory mMemory;
    public:
        Machine(size_t cores, off64_t memorySize = sm::gigabytes(16).bytes());

        static void init();
    };
}
