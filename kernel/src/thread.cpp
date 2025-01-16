#include "thread.hpp"

extern "C" char __tlsdata_start[];
extern "C" char __tlsdata_end[];

struct ThreadLocalData {
    /// @brief Pointer to thread local storage.
    void *tls;
};

[[gnu::section(".tlsroot"), gnu::used]]
static constinit ThreadLocalData tlsThreadData = { };

size_t km::TlsDataSize() {
    return (uintptr_t)__tlsdata_end - (uintptr_t)__tlsdata_start;
}

void *km::AllocateTlsRegion(SystemMemory& memory) {
    size_t size = TlsDataSize();
    return memory.allocate(size, x64::kPageSize);
}

static std::byte *GetTlsBase() {
    std::byte *tls = nullptr;
    asm volatile ("mov %%gs:0, %0" : "=r"(tls));
    return tls;
}

void km::InitTlsRegion(SystemMemory& memory) {
    void *data = AllocateTlsRegion(memory);
    kGsBase.store((uintptr_t)data);
    asm volatile ("mov %0, %%gs:0" :: "r"(data));
}

uint64_t km::GetTlsOffset(const void *object) {
    uintptr_t offset = (uintptr_t)object - (uintptr_t)__tlsdata_start;
    return offset;
}

void *km::GetTlsData(void *object) {
    uintptr_t offset = GetTlsOffset(object);
    std::byte *base = GetTlsBase();
    return base + offset;
}

km::TlsRegisters km::LoadTlsRegisters() {
    uint64_t gsBase = kGsBase.load();
    uint64_t fsBase = kFsBase.load();
    uint64_t kernelGsBase = kKernelGsBase.load();

    return TlsRegisters {
        .fsBase = fsBase,
        .gsBase = gsBase,
        .kernelGsBase = kernelGsBase,
    };
}

void km::StoreTlsRegisters(TlsRegisters registers) {
    kFsBase.store(registers.fsBase);
    kGsBase.store(registers.gsBase);
    kKernelGsBase.store(registers.kernelGsBase);
}
