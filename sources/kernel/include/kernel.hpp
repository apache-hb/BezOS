#pragma once

#include <stddef.h>

#include "log.hpp" // IWYU pragma: export
#include "gdt.hpp"
#include "memory.hpp"
#include "thread.hpp"

namespace km {
    class Scheduler;
    class AddressSpace;

    void SetupApGdt(void);
    void SetupInitialGdt(void);

    Scheduler *GetScheduler();
    SystemMemory *GetSystemMemory();
    AddressSpace& GetProcessPageManager();
    PageTables& GetProcessPageTables();

    CPU_LOCAL
    extern constinit km::CpuLocal<SystemGdt> tlsSystemGdt;

    CPU_LOCAL
    extern constinit km::CpuLocal<x64::TaskStateSegment> tlsTaskState;
}
