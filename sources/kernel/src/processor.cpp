#include "processor.hpp"

#include "apic.hpp"
#include "thread.hpp"

struct KernelThreadData {
    uint32_t lapicId;
    km::Apic pic;
};

CPU_LOCAL
static constinit km::CpuLocal<KernelThreadData> tlsCoreInfo;

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

km::IApic *km::GetCpuLocalApic() {
    return *tlsCoreInfo->pic;
}
