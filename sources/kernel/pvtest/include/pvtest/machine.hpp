#pragma once

#include "pvtest/memory.hpp"
#include "pvtest/cpu.hpp"
#include "util/memory.hpp"

namespace pv {
    class CpuCore;
    class IModelRegisterSet;

    uint64_t getOperand(mcontext_t *mcontext, cs_x86_op operand) noexcept;
    void setRegisterOperand(mcontext_t *mcontext, x86_reg reg, uint64_t value) noexcept;
    uint64_t getRegisterOperand(mcontext_t *mcontext, x86_reg reg) noexcept;

    class Machine {
        km::PageBuilder mPageBuilder;
        Memory mMemory;
        std::vector<CpuCore, SharedAllocator<CpuCore>> mCores;

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

        void emulateReadControl(mcontext_t *mcontext, cs_insn *insn, x86_reg reg, uint64_t value);
        void emulateWriteControl(mcontext_t *mcontext, cs_insn *insn, x86_reg reg, uint64_t value);

        void emulate_mov(mcontext_t *mcontext, cs_insn *insn);

        void emulate_xgetbv(mcontext_t *mcontext, cs_insn *insn);
        void emulate_xsetbv(mcontext_t *mcontext, cs_insn *insn);

    public:
        Machine(size_t cores, off64_t memorySize = sm::gigabytes(16).bytes());
        virtual ~Machine();

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

        virtual uint64_t xgetbv(uint32_t) { return 0; }
        virtual void xsetbv(uint32_t, uint64_t) { }

        virtual uint64_t rdmsr(uint32_t) { return 0; }
        virtual void wrmsr(uint32_t, uint64_t) { }

        virtual uint64_t readCr0() { return 0; }
        virtual uint64_t readCr3() { return 0; }
        virtual uint64_t readCr4() { return 0; }

        virtual void writeCr0(uint64_t) { }
        virtual void writeCr3(uint64_t) { }
        virtual void writeCr4(uint64_t) { }
    };
}
