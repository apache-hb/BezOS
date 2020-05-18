#include "boot/boot.h"

int i;

void kmain(bootinfo_t* info)
{
    i = info->parts;
}
