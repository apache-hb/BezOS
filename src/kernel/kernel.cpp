#include "common/types.h"
#include "common/vga.h"
#include "common/cpuid.h"

#include "utils/stdlib.h"

void enable_ext()
{
    {
        auto id = bezos::cpuid(0);
        char vendor[13] = {};

        // print vendor string because we can
        bezos::copy(vendor, &id.ebx, 4);
        bezos::copy(vendor + 4, &id.edx, 4);
        bezos::copy(vendor + 8, &id.ecx, 4);

        bezos::vga::print(vendor);
    }

    {
        auto id = bezos::cpuid(1);

        struct {
            bezos::u8 step:4;
            bezos::u8 model:4;
            bezos::u8 family:4;
            bezos::u8 type:2;
            bezos::u8 pad1:2;
            bezos::u8 extmodel:4;
            bezos::u8 extfamily:8;
            bezos::u8 pad2:4;
        } info;
        
        info = *reinterpret_cast<decltype(info)*>(&id.eax);

        bezos::vga::print("\nstepping: "); bezos::vga::print(info.step, 10);
        bezos::vga::print("\nmodel: "); bezos::vga::print(info.model, 10);
    }
}

extern "C" void kmain(void)
{
    bezos::u64 i = *(bezos::u64*)0xB8000;
    i++;
    bezos::vga::print(i);
    bezos::vga::print("\n64 bit\n");
    enable_ext();
    while(true);
}