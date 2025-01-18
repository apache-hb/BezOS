#include "processor.hpp"

#include "apic.hpp"
#include "thread.hpp"

struct KernelThreadData {
    uint32_t lapicId;
    km::LocalApic lapic;
    // km::IntController pic;
};

[[gnu::section(".tlsdata")]]
static constinit km::ThreadLocal<KernelThreadData> tlsCoreInfo;

void km::InitKernelThread(LocalApic lapic) {
    uint32_t id = lapic.id();
    tlsCoreInfo = KernelThreadData {
        .lapicId = id,
        .lapic = lapic,
    };
}

km::CpuCoreId km::GetCurrentCoreId() {
    return CpuCoreId(tlsCoreInfo->lapicId);
}

km::LocalApic km::GetCurrentCoreApic() {
    return tlsCoreInfo->lapic;
}

// km::IntController km::GetCurrentCoreIntController() {
//     return tlsCoreInfo->pic;
// }
