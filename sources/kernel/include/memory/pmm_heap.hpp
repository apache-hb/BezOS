#pragma once

#include "memory/heap.hpp"

namespace km {
    using PmmAllocation = GenericTlsfAllocation<PhysicalAddress>;
    using PmmHeap = GenericTlsfHeap<PhysicalAddress>;
}
