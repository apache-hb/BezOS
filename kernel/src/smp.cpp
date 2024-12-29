#include "smp.hpp"

#include <stdint.h>

struct [[gnu::packed]] SmpInfo {
    uint32_t pml4;
    uint32_t stackBase;
    uint32_t stackSize;
    uint32_t entry;
};

extern const char _binary_smp_bin_start[];
extern const char _binary_smp_bin_end[];

void KmInitSmp(void) {
    [[maybe_unused]]
    volatile int i = (uintptr_t)_binary_smp_bin_end - (uintptr_t)_binary_smp_bin_start;
}

extern "C" void KmSmpStartup(void) {

}
