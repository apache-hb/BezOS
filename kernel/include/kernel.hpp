#pragma once

#include <stddef.h>

#include "log.hpp" // IWYU pragma: export
#include "gdt.hpp"
#include "thread.hpp"

namespace km {
    void SetupApGdt(void);
    void SetupInitialGdt(void);

    [[gnu::section(".tlsdata")]]
    extern constinit km::ThreadLocal<SystemGdt> tlsSystemGdt;

    [[gnu::section(".tlsdata")]]
    extern constinit km::ThreadLocal<x64::TaskStateSegment> tlsTaskState;
}
