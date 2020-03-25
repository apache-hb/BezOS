#include "vesa.h"

#include "vga/vga.h"

#include "common/types.h"

namespace vesa
{
    
    struct __attribute__((packed)) vbe_info
    {
        char signature[4];
        uint16 version;
        uint32 oem;
        uint32 caps;

        uint16 modes_offset;
        uint16 modes_segment;

        uint16 memory;
        uint16 revision;
        uint32 vendor;
        uint32 name;
        uint32 rev;

        pad<222> reserved;

        char oem_data[256];
    };

    static_assert(sizeof(vbe_info) == 512);

    extern "C" uint32 LOW_MEMORY;
    extern "C" uint32 VBE_INFO;
    extern "C" uint32 VBE_MAP;

    void init()
    {
        vga::puti(LOW_MEMORY, 16);
        vga::putc('\n');
        vga::puti(VBE_INFO, 16);
        vbe_info* info = reinterpret_cast<vbe_info*>(VBE_INFO);
        vga::putc('\n');
        vga::puti((uint64)info, 16);

        vga::putc(info->signature[0]);
        vga::putc(info->signature[1]);
        vga::putc(info->signature[2]);
        vga::putc(info->signature[3]);
    }
}