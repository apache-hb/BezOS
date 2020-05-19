#include "boot/boot.h"

#if !defined(__x86_64__)
#   error "code must be compiled for 32 bit"
#endif


int i;

void kmain(bootinfo_t* info)
{
    i = info->parts;
}
