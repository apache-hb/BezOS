#pragma once

#include <stddef.h>

#include "log.hpp" // IWYU pragma: export
#include "gdt.hpp"
#include "memory.hpp"
#include "thread.hpp"

namespace sys {
    class System;
    class AddressSpaceManager;
}

namespace km {
    class Scheduler;
    class AddressSpace;

    void SetupApGdt(void);
    void SetupInitialGdt(void);

    Scheduler *GetScheduler();
    SystemMemory *GetSystemMemory();
    sys::AddressSpaceManager& GetProcessPageManager();
    PageTables& GetProcessPageTables();
    sys::System *GetSysSystem();

    CPU_LOCAL
    extern constinit km::CpuLocal<SystemGdt> tlsSystemGdt;

    CPU_LOCAL
    extern constinit km::CpuLocal<x64::TaskStateSegment> tlsTaskState;
}
