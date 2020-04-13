#include "mm/mm.h"

extern "C" void kmain(BootData* data)
{
    mm::init(data->memory);
}