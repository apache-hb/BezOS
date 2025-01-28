#include "processor.hpp"

#include "apic.hpp"
#include "thread.hpp"

struct KernelThreadData {
    uint32_t lapicId;
    km::LocalApic lapic;
    km::Apic pic;
};

[[gnu::section(".tlsdata")]]
static constinit km::ThreadLocal<KernelThreadData> tlsCoreInfo;

void km::InitKernelThread(Apic pic) {
    uint32_t id = pic->id();
    tlsCoreInfo = KernelThreadData {
        .lapicId = id,
        .pic = pic,
    };
}

km::CpuCoreId km::GetCurrentCoreId() {
    return CpuCoreId(tlsCoreInfo->lapicId);
}

km::IApic *km::GetCurrentCoreApic() {
    return *tlsCoreInfo->pic;
}
