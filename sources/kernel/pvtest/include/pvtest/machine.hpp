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
        CpuCore *getCore(size_t index) {
            return &mCores[index];
        }

        void bspInit(mcontext_t mcontext);

        template<typename F>
        void bspLaunch(F &&func) {
            void *arg = std::bit_cast<void*>(&func);
            void *(*invoke)(void*) = [](void *arg) -> void* {
                auto func = std::bit_cast<F*>(arg);
                (*func)();
                return nullptr;
            };

            x64::page *stack = (x64::page*)pv::SharedObjectAlignedAlloc(alignof(x64::page), sizeof(x64::page) * 4);
            char *base = (char*)(stack + 4);
            *(uint64_t*)base = 0;
            bspInit({
                .gregs = {
                    [REG_RIP] = (greg_t)invoke,
                    [REG_RDI] = (greg_t)arg,
                    [REG_RSP] = (greg_t)(base - 0x8),
                    [REG_RBP] = (greg_t)(base - 0x8),
                }
            });
        }

        PVTEST_SHARED_OBJECT(Machine);

        static km::VirtualRangeEx getSharedMemory();

        static void init();
        static void finalize();

        static void initChild();
        static void finalizeChild();
    };
}
