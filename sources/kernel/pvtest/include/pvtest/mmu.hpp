#pragma once

#include <stdint.h>

namespace pv {
    class Memory;

    class CpuMmu {
        uintptr_t mCr3;
        Memory *mMemory;

    public:
        CpuMmu(Memory *memory);

        void setCr3(uintptr_t cr3);
        void fault(uintptr_t address);
        void invlpg(uintptr_t address);
    };
}
