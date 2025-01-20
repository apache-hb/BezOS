#include "processor.hpp"

#include "apic.hpp"
#include "thread.hpp"

struct KernelThreadData {
    uint32_t lapicId;
    km::LocalApic lapic;
    km::IntController pic;
};

[[gnu::section(".tlsdata")]]
static constinit km::ThreadLocal<KernelThreadData> tlsCoreInfo;

void km::InitKernelThread(IntController pic) {
    uint32_t id = pic->id();
    tlsCoreInfo = KernelThreadData {
        .lapicId = id,
        .pic = pic,
    };
}

km::CpuCoreId km::GetCurrentCoreId() {
    return CpuCoreId(tlsCoreInfo->lapicId);
}

km::IIntController *km::GetCurrentCoreIntController() {
    return *tlsCoreInfo->pic;
}
