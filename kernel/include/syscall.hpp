#pragma once

#include "process.hpp"

#include <cstdint>

namespace km {
    using SystemCallHandler = uint64_t(*)(uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3);

    struct [[gnu::packed]] SystemCallContext {
        // user registers
        // r15 is clobbered by syscall
        uint64_t r14;
        uint64_t r13;
        uint64_t r12;
        // r11 is clobbered by syscall
        uint64_t r10;
        // r9 is clobbered by syscall
        // r8 is a parameter
        uint64_t rbx;
        uint64_t rbp;

        // syscall related
        uint64_t rip;
        uint64_t rflags;
        uint64_t arg3;
        uint64_t arg2;
        uint64_t arg1;
        uint64_t arg0;
        uint64_t function;
        uint64_t userStack;
    };

    void SetupUserMode(mem::IAllocator *allocator, uint8_t ist);

    void EnterUserMode(x64::RegisterState state);

    void AddSystemCall(uint8_t function, SystemCallHandler handler);
}
