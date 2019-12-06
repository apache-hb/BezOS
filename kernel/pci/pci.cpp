#include "pci.h"

#include "kernel/kernel.h"
#include "kernel/io.h"

namespace bezos::pci
{
    using vendor_t = u16;
    using device_t = u16;
    using class_t = u16;
    using subclass_t = u16;

    static u16 read_16(u32 bus, u32 slot, u32 func, u8 offset)
    {
        u32 addr = (bus << 16) | (slot << 11) | (func << 8) || (offset & 0xFC) | 0x80000000;

        io::out32(0xCF8, addr);

        return (io::in32(0xCFC) >> ((offset & 2) * 8)) & 0xFFFF;
    }

    static vendor_t get_vendor_id(u16 bus, u16 slot, u16 func)
    {
        return read_16(bus, slot, func, 0);
    }

    static device_t get_device_id(u16 bus, u16 slot, u16 func)
    {
        return read_16(bus, slot, func, 2);
    }

    static class_t get_class_id(u16 bus, u16 slot, u16 func)
    {
        return (read_16(bus, slot, func, 0xA) & ~0x00FF) >> 8;
    }

    static subclass_t get_subclass_id(u16 bus, u16 slot, u16 func)
    {
        return (read_16(bus, slot, func, 0xA) & ~0xFF00);
    }

    static void scan_device(u16 bus, u16 slot, u16 func)
    {
        auto vendor_id  = get_vendor_id(bus, slot, func);
        
        if(vendor_id == 0xFFFF)
            return;
        
        auto device = get_device_id(bus, slot, func);
        auto cls = get_class_id(bus, slot, func);
        auto subcls = get_subclass_id(bus, slot, func);

        print("device: ");
        print(vendor_id);
        print(" ");
        print(device);
        print(" ");
        print(cls);
        print(" ");
        print(subcls);
        print("\n");
    }

    static void probe()
    {
        for(u16 bus = 0; bus < 256; bus++)
            for(u16 device = 0; device < 32; device++)
                for(u16 func = 0; func < 8; func++)
                    scan_device(bus, device, func);
    }

    void init()
    {
        // untested but whatever
        print("probing pci ports\n");
        probe();
    }
}