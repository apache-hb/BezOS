#include "memory/physical.hpp"

#include "check.h"

#include <limine.h>

using namespace km;

PhysicalMemoryLayout::PhysicalMemoryLayout(const struct limine_memmap_response *memmap) {
    KM_CHECK(memmap != nullptr, "No memory map!");

    mUsedAvailableMemoryRanges = 0;
}
