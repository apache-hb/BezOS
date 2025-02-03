#pragma once

#include "std/vector.hpp"

#include "arch/arch.hpp"

#include <cstdint>

namespace km {
    struct ProcessThread {
        uint32_t threadId;
        x64::RegisterState regs;
        x64::MachineState machine;
    };

    struct Process {
        uint32_t processId;
        stdx::Vector2<void*> memory;
        ProcessThread main;
    };
}
