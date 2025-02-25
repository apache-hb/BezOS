#include "memory/pte.hpp"

#include "log.hpp"
#include "memory/paging.hpp"

#include <limits.h>

using namespace km;

extern "C" {
    extern char __text_start[]; // always aligned to 0x1000
    extern char __text_end[];

    extern char __rodata_start[]; // always aligned to 0x1000
    extern char __rodata_end[];

    extern char __data_start[]; // always aligned to 0x1000
    extern char __data_end[];

    extern char __kernel_start[];
    extern char __kernel_end[];
}

static void MapKernelPages(PageTableManager& memory, km::PhysicalAddress paddr, const void *vaddr) {
    auto mapKernelRange = [&](const void *begin, const void *end, PageFlags flags, stdx::StringView name) {
        km::PhysicalAddress front = km::PhysicalAddress {  (uintptr_t)begin - (uintptr_t)__kernel_start };
        km::PhysicalAddress back = km::PhysicalAddress { (uintptr_t)end - (uintptr_t)__kernel_start };

        KmDebugMessage("[INIT] Mapping ", Hex((uintptr_t)begin), "-", Hex((uintptr_t)end), " - ", name, "\n");

        memory.mapRange({ paddr + front.address, paddr + back.address }, (char*)vaddr + front.address, flags, MemoryType::eWriteBack);
    };

    KmDebugMessage("[INIT] Mapping kernel pages.\n");

    mapKernelRange(__text_start, __text_end, PageFlags::eCode, ".text");
    mapKernelRange(__rodata_start, __rodata_end, PageFlags::eRead, ".rodata");
    mapKernelRange(__data_start, __data_end, PageFlags::eData, ".data");
}

void KmMapKernel(km::PageTableManager& vmm, km::PhysicalAddress paddr, const void *vaddr) {
    // first map the kernel pages
    MapKernelPages(vmm, paddr, vaddr);
}

void KmUpdateRootPageTable(const km::PageBuilder& pm, km::PageTableManager& vmm) {
    KmDebugMessage("[INIT] PML4: ", vmm.rootPageTable(), "\n");
    pm.setActiveMap(vmm.rootPageTable());
}
