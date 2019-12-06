#include "kernel/kernel.h"

extern "C" void _kernel_bsp(void)
{
    bezos::kernel_main();
    while(true);
}

extern "C" void _kernel_ap(void)
{

}