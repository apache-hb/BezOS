#include "processor.hpp"

#include "apic.hpp"
#include "thread.hpp"

struct KernelThreadData {
    uint32_t lapicId;
    km::Apic apic;
};

CPU_LOCAL
static constinit km::CpuLocal<KernelThreadData> tlsCoreInfo;

void km::InitKernelThread(Apic apic) {
    uint32_t id = apic->id();
    tlsCoreInfo = KernelThreadData {
        .lapicId = id,
        .apic = apic,
    };
}

km::CpuCoreId km::GetCurrentCoreId() {
    return CpuCoreId(tlsCoreInfo->lapicId);
}

km::IApic *km::GetCpuLocalApic() {
    return *tlsCoreInfo->apic;
}
