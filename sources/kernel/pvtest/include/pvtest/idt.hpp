#pragma once

#include <sys/ucontext.h>

#include "arch/intrin.hpp"

namespace x64 {
    struct TaskStateSegment;
}

namespace pv {
    class CpuIdt {
        GDTR mGdtPointer{};
        IDTR mIdtPointer{};
        uint16_t mTssSelector{0xFFFF};

        x64::TaskStateSegment *getTaskStateSegment();

        void invokeExceptionHandler(mcontext_t *mcontext, uintptr_t rip, uintptr_t rsp);

        void doublefault(mcontext_t *mcontext);

    public:
        void ltr(uint16_t selector);
        void lgdt(GDTR ptr);
        void lidt(IDTR ptr);

        void raise(mcontext_t *mcontext, uint8_t vector);
    };
}
