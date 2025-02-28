#include "memory/pte.hpp"

#include "log.hpp"
#include "memory/paging.hpp"

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

static void MapKernelPages(PageTables& memory, km::PhysicalAddress paddr, const void *vaddr) {
    auto mapKernelRange = [&](const void *begin, const void *end, PageFlags flags, stdx::StringView name) {
        PhysicalAddress front = (uintptr_t)begin - (uintptr_t)__kernel_start;
        PhysicalAddress back = (uintptr_t)end - (uintptr_t)__kernel_start;

        MemoryRange range = { paddr + front.address, paddr + back.address };
        const void *section = (char*)vaddr + front.address;

        AddressMapping mapping = MappingOf(range, section);
        if (!mapping.alignsExactlyTo(x64::kPageSize)) {
            KmDebugMessage("[INIT] Damaged kernel image detected: ", name, " is not page aligned.\n");
            KmDebugMessage("[DATA] ", mapping, "\n");
            KM_PANIC("Kernel image is not page aligned.");
        }

        AddressMapping aligned = mapping.aligned(x64::kPageSize);

        KmDebugMessage("[INIT] Mapping ", aligned, " - ", name, "\n");

        memory.map(aligned, flags, MemoryType::eWriteBack);
    };

    KmDebugMessage("[INIT] Mapping kernel pages.\n");

    mapKernelRange(__text_start, __text_end, PageFlags::eCode, ".text");
    mapKernelRange(__rodata_start, __rodata_end, PageFlags::eRead, ".rodata");
    mapKernelRange(__data_start, __data_end, PageFlags::eData, ".data");
}

void km::MapKernel(km::PageTables& vmm, km::PhysicalAddress paddr, const void *vaddr) {
    MapKernelPages(vmm, paddr, vaddr);
}

void km::UpdateRootPageTable(const km::PageBuilder& pm, km::PageTables& vmm) {
    KmDebugMessage("[INIT] PML4: ", vmm.root(), "\n");
    pm.setActiveMap(vmm.root());
}
