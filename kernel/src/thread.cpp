#include "thread.hpp"

extern "C" char __tlsdata_start[];
extern "C" char __tlsdata_end[];

struct ThreadLocalData {
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

void *km::GetTlsData(void *object) {
    uintptr_t offset = (uintptr_t)object - (uintptr_t)__tlsdata_start;
    std::byte *base = GetTlsBase();
    return base + offset;
}
