#pragma once

#include <pthread.h>
#include <setjmp.h>
#include <capstone.h>
#include <sys/ucontext.h>

#include "arch/intrin.hpp"
#include "common/util/util.hpp"
#include "common/virtual_address.hpp"
#include "common/physical_address.hpp"
#include "pvtest/system.hpp"

namespace pv {
    class Memory;

    class CpuCore {
        std::unique_ptr<std::byte[]> mChildStack;
        pid_t mChild;
        jmp_buf mDestroyTarget;
        jmp_buf mLaunchTarget;
        mcontext_t mLaunchContext;
        stack_t mSigStack;

        uint64_t cr0;
        uint64_t cr2;
        uint64_t cr3;
        uint64_t cr4;
        uint8_t cpl;

        IDTR idtr;
        GDTR gdtr;
        uint32_t eflags;

        uint64_t fs_base;
        uint64_t gs_base;
        uint64_t kernel_gs_base;

        static int start(void *arg);
        void installSignals();

        void run();

        void sigsegv(mcontext_t *mcontext);
        void launch(mcontext_t *mcontext);
        void destroy(mcontext_t *mcontext);

        void emulateCli(mcontext_t *mcontext, cs_insn *insn);
        void emulateSti(mcontext_t *mcontext, cs_insn *insn);
        void emulateLidt(mcontext_t *mcontext, cs_insn *insn);
        void emulateLgdt(mcontext_t *mcontext, cs_insn *insn);
        void emulateLtr(mcontext_t *mcontext, cs_insn *insn);
        void emulateInb(mcontext_t *mcontext, cs_insn *insn);
        void emulateInw(mcontext_t *mcontext, cs_insn *insn);
        void emulateInd(mcontext_t *mcontext, cs_insn *insn);
        void emulateOutb(mcontext_t *mcontext, cs_insn *insn);
        void emulateOutw(mcontext_t *mcontext, cs_insn *insn);
        void emulateOutd(mcontext_t *mcontext, cs_insn *insn);
        void emulateInvlpg(mcontext_t *mcontext, cs_insn *insn);
        void emulateSwapgs(mcontext_t *mcontext, cs_insn *insn);
        void emulateRdmsr(mcontext_t *mcontext, cs_insn *insn);
        void emulateWrmsr(mcontext_t *mcontext, cs_insn *insn);
        void emulateHlt(mcontext_t *mcontext, cs_insn *insn);

    public:
        UTIL_NOCOPY(CpuCore);
        UTIL_NOMOVE(CpuCore);

        CpuCore();
        ~CpuCore();

        PVTEST_SHARED_OBJECT(CpuCore);

        void setRegisterOperand(mcontext_t *mcontext, x86_reg reg, uint64_t value) noexcept;
        uint64_t getRegisterOperand(mcontext_t *mcontext, x86_reg reg) noexcept;
        uint64_t getMemoryOperand(mcontext_t *mcontext, const x86_op_mem *op) noexcept;

        sm::PhysicalAddress resolveVirtualAddress(Memory *memory, sm::VirtualAddress address) noexcept;
    };
}
