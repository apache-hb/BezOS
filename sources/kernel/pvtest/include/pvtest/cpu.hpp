#pragma once

#include <pthread.h>
#include <setjmp.h>
#include <capstone.h>
#include <sys/ucontext.h>

#include "common/util/util.hpp"
#include "common/virtual_address.hpp"
#include "common/physical_address.hpp"
#include "pvtest/system.hpp"

namespace pv {
    class Memory;

    class CpuCore {
        std::unique_ptr<std::byte[]> mStack;
        pid_t mChild;
        jmp_buf mDestroyTarget;
        jmp_buf mLaunchTarget;

        uint64_t cr0;
        uint64_t cr2;
        uint64_t cr3;
        uint64_t cr4;
        uint8_t cpl;

        static int start(void *arg);

        void run();

        void sigsegv(mcontext_t *mcontext);
        void launch(mcontext_t *mcontext);
        void destroy(mcontext_t *mcontext);

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

        static void installSignals();
    };
}
