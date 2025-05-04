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
        csh mCapstone;
        cs_insn *mInstruction;

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
        void destroy(mcontext_t *mcontext);

        void emulate_cli(mcontext_t *mcontext, cs_insn *insn);
        void emulate_sti(mcontext_t *mcontext, cs_insn *insn);
        void emulate_lidt(mcontext_t *mcontext, cs_insn *insn);
        void emulate_lgdt(mcontext_t *mcontext, cs_insn *insn);
        void emulate_ltr(mcontext_t *mcontext, cs_insn *insn);
        void emulate_inb(mcontext_t *mcontext, cs_insn *insn);
        void emulate_inw(mcontext_t *mcontext, cs_insn *insn);
        void emulate_ind(mcontext_t *mcontext, cs_insn *insn);
        void emulate_outb(mcontext_t *mcontext, cs_insn *insn);
        void emulate_outw(mcontext_t *mcontext, cs_insn *insn);
        void emulate_outd(mcontext_t *mcontext, cs_insn *insn);
        void emulate_invlpg(mcontext_t *mcontext, cs_insn *insn);
        void emulate_swapgs(mcontext_t *mcontext, cs_insn *insn);
        void emulate_rdmsr(mcontext_t *mcontext, cs_insn *insn);
        void emulate_wrmsr(mcontext_t *mcontext, cs_insn *insn);
        void emulate_hlt(mcontext_t *mcontext, cs_insn *insn);

    public:
        UTIL_NOCOPY(CpuCore);
        UTIL_NOMOVE(CpuCore);

        CpuCore();
        ~CpuCore();

        PVTEST_SHARED_OBJECT(CpuCore);

        void initMessage(mcontext_t context);

        void setRegisterOperand(mcontext_t *mcontext, x86_reg reg, uint64_t value) noexcept;
        uint64_t getRegisterOperand(mcontext_t *mcontext, x86_reg reg) noexcept;
        uint64_t getMemoryOperand(mcontext_t *mcontext, const x86_op_mem *op) noexcept;

        sm::PhysicalAddress resolveVirtualAddress(Memory *memory, sm::VirtualAddress address) noexcept;
    };
}
