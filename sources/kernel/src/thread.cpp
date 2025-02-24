#include "thread.hpp"

extern "C" char __cpudata_start[];
extern "C" char __cpudata_end[];

struct CpuLocalData {
    /// @brief Pointer to cpu local storage.
    void *tls;
};

[[gnu::section(".cpuroot"), gnu::used]]
static constinit CpuLocalData tlsThreadData = { };

size_t km::CpuLocalDataSize() {
    return (uintptr_t)__cpudata_end - (uintptr_t)__cpudata_start;
}

void *km::AllocateCpuLocalRegion() {
    size_t size = CpuLocalDataSize();
    return malloc(size);
}

static std::byte *GetCpuLocalBase() {
    KM_CHECK(km::IsCpuStorageSetup(), "CPU local storage not setup.");

    std::byte *tls = nullptr;
    asm volatile ("mov %%gs:0, %0" : "=r"(tls));
    return tls;
}

void km::InitCpuLocalRegion() {
    void *data = AllocateCpuLocalRegion();
    kGsBase.store((uintptr_t)data);
    asm volatile ("mov %0, %%gs:0" :: "r"(data));
}

uint64_t km::GetCpuStorageOffset(const void *object) {
    uintptr_t offset = (uintptr_t)object - (uintptr_t)__cpudata_start;
    return offset;
}

void *km::GetCpuLocalData(void *object) {
    uintptr_t offset = GetCpuStorageOffset(object);
    std::byte *base = GetCpuLocalBase();
    return base + offset;
}

bool km::IsCpuStorageSetup() {
    return kGsBase.load() != 0;
}

km::CpuLocalRegisters km::LoadTlsRegisters() {
    uint64_t gsBase = kGsBase.load();
    uint64_t fsBase = kFsBase.load();
    uint64_t kernelGsBase = kKernelGsBase.load();

    return CpuLocalRegisters {
        .fsBase = fsBase,
        .gsBase = gsBase,
        .kernelGsBase = kernelGsBase,
    };
}

void km::StoreTlsRegisters(CpuLocalRegisters registers) {
    kFsBase.store(registers.fsBase);
    kGsBase.store(registers.gsBase);
    kKernelGsBase.store(registers.kernelGsBase);
}
