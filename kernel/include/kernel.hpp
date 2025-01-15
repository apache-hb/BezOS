#pragma once

#include <stddef.h>

#include "apic.hpp"
#include "util/format.hpp"

#include "gdt.hpp"
#include "thread.hpp"

void KmBeginWrite();
void KmEndWrite();

km::IOutStream *GetDebugStream();
void SetupApGdt(void);
void SetupInitialGdt(void);

namespace km {
    enum class CpuCoreId : uint32_t {
        eInvalid = 0xFFFF'FFFF
    };

    struct KernelThreadData {
        void *syscallStack;
        uint32_t lapicId;
    };

    [[gnu::section(".tlsdata")]]
    extern constinit km::ThreadLocal<KernelThreadData> tlsKernelThreadData;

    [[gnu::section(".tlsdata")]]
    extern constinit km::ThreadLocal<km::LocalApic> tlsLocalApic;

    [[gnu::section(".tlsdata")]]
    extern constinit km::ThreadLocal<SystemGdt> tlsSystemGdt;

    [[gnu::section(".tlsdata")]]
    extern constinit km::ThreadLocal<x64::TaskStateSegment> tlsTaskState;

    CpuCoreId GetCurrentCoreId();
}
