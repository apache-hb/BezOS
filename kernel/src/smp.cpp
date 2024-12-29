#include "smp.hpp"

#include <stdint.h>

struct [[gnu::packed]] SmpInfo {
    uint32_t pml4;
    uint32_t stackBase;
    uint32_t stackSize;
    uint32_t entry;
};

extern "C" void KmSmpStartup(void) {

}
