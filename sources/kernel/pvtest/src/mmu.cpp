#include "pvtest/mmu.hpp"

pv::CpuMmu::CpuMmu(Memory *memory)
    : mCr3(0)
    , mMemory(memory)
{ }

void pv::CpuMmu::setCr3(uintptr_t cr3) {

}

void pv::CpuMmu::fault(uintptr_t address) {

}

void pv::CpuMmu::invlpg(uintptr_t address) {

}
