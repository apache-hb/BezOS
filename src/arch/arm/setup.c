#include <stdint.h>
#define UART0_BASE 0x1c090000

void setup() 
{
    *(volatile uint32_t*)(UART0_BASE) = 'A';
}