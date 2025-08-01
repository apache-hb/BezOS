#pragma once

#include "pvtest/memory.hpp"
#include "pvtest/cpu.hpp"
#include "util/memory.hpp"

namespace pv {
    class CpuCore;
    class IModelRegisterSet;

    class Machine {
        km::PageBuilder mPageBuilder;
        Memory mMemory;
        std::vector<CpuCore, SharedAllocator<CpuCore>> mCores;
        IModelRegisterSet *mMsrSet = nullptr;

        csh mCapstone{};
        cs_insn *mInstruction{};
        stack_t mSigStack{};

        void sigsegv(mcontext_t *mcontext);

        void emulate_cli(mcontext_t *mcontext, cs_insn *insn);
        void emulate_sti(mcontext_t *mcontext, cs_insn *insn);
        void emulate_lidt(mcontext_t *mcontext, cs_insn *insn);
        void emulate_lgdt(mcontext_t *mcontext, cs_insn *insn);
        void emulate_ltr(mcontext_t *mcontext, cs_insn *insn);
        void emulate_in(mcontext_t *mcontext, cs_insn *insn);
        void emulate_out(mcontext_t *mcontext, cs_insn *insn);
        void emulate_invlpg(mcontext_t *mcontext, cs_insn *insn);
        void emulate_swapgs(mcontext_t *mcontext, cs_insn *insn);
        void emulate_rdmsr(mcontext_t *mcontext, cs_insn *insn);
        void emulate_wrmsr(mcontext_t *mcontext, cs_insn *insn);
        void emulate_hlt(mcontext_t *mcontext, cs_insn *insn);
        void emulate_iretq(mcontext_t *mcontext, cs_insn *insn);
        void emulate_mmu(mcontext_t *mcontext, cs_insn *insn);

    public:
        Machine(size_t cores, off64_t memorySize = sm::gigabytes(16).bytes(), IModelRegisterSet *msrs = nullptr);
        ~Machine();

        km::PageBuilder *getPageBuilder() { return &mPageBuilder; }
        Memory *getMemory() { return &mMemory; }
        CpuCore *getCore(size_t index) {
            return &mCores[index];
        }

        void initSingleThread();

        void bspInit(mcontext_t mcontext);

        template<typename F>
        void bspLaunch(F &&func) {
            void *arg = std::bit_cast<void*>(&func);
            void *(*invoke)(void*) = [](void *arg) -> void* {
                auto func = std::bit_cast<F*>(arg);
                (*func)();
                return nullptr;
            };

            x64::page *stack = (x64::page*)pv::sharedObjectAlignedAlloc(alignof(x64::page), sizeof(x64::page) * 4);
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
