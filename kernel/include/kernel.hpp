#pragma once

#include <stddef.h>

#include "log.hpp" // IWYU pragma: export
#include "gdt.hpp"
#include "thread.hpp"

void SetupApGdt(void);
void SetupInitialGdt(void);

namespace km {
    [[gnu::section(".tlsdata")]]
    extern constinit km::ThreadLocal<SystemGdt> tlsSystemGdt;

    [[gnu::section(".tlsdata")]]
    extern constinit km::ThreadLocal<x64::TaskStateSegment> tlsTaskState;
}
