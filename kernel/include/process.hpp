#pragma once

#include "std/vector.hpp"

#include <cstdint>

namespace km {
    struct [[gnu::packed]] MachineState {
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
        uint64_t rsp;
        uint64_t rip;
        uint64_t rflags;
    };

    struct ProcessThread {
        uint32_t threadId;
        MachineState state;
    };

    struct Process {
        uint32_t processId;
        stdx::Vector<void*> memory;
        ProcessThread main;
    };
}
