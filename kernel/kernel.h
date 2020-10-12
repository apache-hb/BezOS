#pragma once

#include <mm/pmm.h>
#include <mm/vmm.h>

extern "C" void kmain(pmm::memory_map memory, vmm::pml4 pml4);
using kmain_t = decltype(kmain);
