#pragma once

#include "pvtest/memory.hpp"
#include "pvtest/cpu.hpp"
#include "util/memory.hpp"

namespace pv {
    class CpuCore;

    class Machine {
        km::PageBuilder mPageBuilder;
        Memory mMemory;
        std::vector<CpuCore, SharedAllocator<CpuCore>> mCores;
    public:
        Machine(size_t cores, off64_t memorySize = sm::gigabytes(16).bytes());

        km::PageBuilder *getPageBuilder() { return &mPageBuilder; }
        Memory *getMemory() { return &mMemory; }

        void bspInit(mcontext_t mcontext);

        PVTEST_SHARED_OBJECT(Machine);

        static km::VirtualRangeEx getSharedMemory();

        static void init();
        static void reset();
        static void finalize();

        static void initChild();
        static void finalizeChild();
    };
}
