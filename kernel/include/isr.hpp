#pragma once

#include "util/util.hpp"
#include <cstddef>
#include <cstdint>
#include <climits>

namespace km {
    struct [[gnu::packed]] IsrContext {
        uint64_t rax;
        uint64_t rbx;
        uint64_t rcx;
        uint64_t rdx;
        uint64_t rdi;
        uint64_t rsi;
        uint64_t r8;
        uint64_t r9;
        uint64_t r10;
        uint64_t r11;
        uint64_t r12;
        uint64_t r13;
        uint64_t r14;
        uint64_t r15;
        uint64_t rbp;

        uint64_t vector;
        uint64_t error;

        uint64_t rip;
        uint64_t cs;
        uint64_t rflags;
        uint64_t rsp;
        uint64_t ss;
    };

    class IsrAllocator {
        static constexpr size_t kIsrCount = 256;
        uint8_t mFreeIsrs[kIsrCount / CHAR_BIT] = {};
    public:
        void claimIsr(uint8_t isr);
        void releaseIsr(uint8_t isr);

        uint8_t allocateIsr();
    };

    void DisableNmi();
    void EnableNmi();
    void DisableInterrupts();
    void EnableInterrupts();

    class IntGuard {
    public:
        UTIL_NOMOVE(IntGuard);
        UTIL_NOCOPY(IntGuard);

        IntGuard() {
            DisableInterrupts();
        }

        ~IntGuard() {
            EnableInterrupts();
        }
    };

    using KmIsrHandler = void*(*)(km::IsrContext*);

    void InitInterrupts(km::IsrAllocator& isrs, uint16_t codeSelector);
    void LoadIdt(void);

    void UpdateIdtEntry(uint8_t isr, uint16_t selector, uint8_t dpl, uint8_t ist);

    KmIsrHandler InstallIsrHandler(uint8_t isr, KmIsrHandler handler);
}
