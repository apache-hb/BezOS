#pragma once

#include "memory/heap.hpp"

namespace km {
    using VmemAllocation = GenericTlsfAllocation<sm::VirtualAddress>;
    using VmemHeap = GenericTlsfHeap<sm::VirtualAddress>;
}
