#pragma once

#include "std/vector.hpp"

#include <cstdint>

namespace km {
    using reg_t = uint64_t;
    using flags_t = uint64_t;

    struct [[gnu::packed]] RegisterState {
        reg_t rax;
        reg_t rbx;
        reg_t rcx;
        reg_t rdx;
        reg_t rdi;
        reg_t rsi;
        reg_t r8;
        reg_t r9;
        reg_t r10;
        reg_t r11;
        reg_t r12;
        reg_t r13;
        reg_t r14;
        reg_t r15;
        uintptr_t rbp;
        uintptr_t rsp;
        uintptr_t rip;
        flags_t rflags;
    };

    struct [[gnu::packed]] MachineState {
        uintptr_t cr3;
    };

    struct ProcessThread {
        uint32_t threadId;
        RegisterState regs;
    };

    struct Process {
        uint32_t processId;
        stdx::Vector<void*> memory;
        ProcessThread main;
    };
}
