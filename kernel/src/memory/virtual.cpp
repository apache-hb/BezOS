#include "memory/virtual.hpp"

using namespace km;

extern "C" {
    extern char __text_start[]; // always aligned to 0x1000
    extern char __text_end[];

    extern char __rodata_start[]; // always aligned to 0x1000
    extern char __rodata_end[];

    extern char __data_start[]; // always aligned to 0x1000
    extern char __data_end[];

    extern char __bss_start[]; // always aligned to 0x1000
    extern char __bss_end[];
}

void RemapKernelMemory(PageAllocator&, limine_kernel_address_response) noexcept {

}
