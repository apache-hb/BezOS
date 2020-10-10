#pragma once

#include <mm/mm.h>

extern "C" void kmain(mm::memory_map memory, mm::pml4 pml4);
using kmain_t = decltype(kmain);
