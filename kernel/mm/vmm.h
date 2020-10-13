#pragma once

#include <util.h>

namespace vmm {
    using pte = u64;
    using pt = pte*;
    using pd = pt*;
    using pdpt = pd*;
    using pml4 = pdpt*;

    void init();
    void map4k(u64 phys, u64 virt);
}
