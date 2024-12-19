#pragma once

#include "memory/physical.hpp"

#include <stdint.h>

namespace km {
    class VirtualMemory {
    public:
        PhysicalAddress virtualToPhysical(VirtualAddress address) const noexcept;
        VirtualAddress physicalToVirtual(PhysicalAddress address) const noexcept;
    };
}
