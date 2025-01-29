#pragma once

#include <stddef.h>

#include "log.hpp" // IWYU pragma: export
#include "gdt.hpp"
#include "thread.hpp"

namespace km {
    void SetupApGdt(void);
    void SetupInitialGdt(void);

    CPU_LOCAL
    extern constinit km::CpuLocal<SystemGdt> tlsSystemGdt;

    CPU_LOCAL
    extern constinit km::CpuLocal<x64::TaskStateSegment> tlsTaskState;
}
