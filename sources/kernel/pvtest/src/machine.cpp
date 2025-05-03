#include "pvtest/machine.hpp"

pv::Machine::Machine(size_t cores, off64_t memorySize)
    : mCores(cores)
    , mMemory(memorySize)
{ }

void pv::Machine::init() {
    pv::CpuCore::installSignals();
}
