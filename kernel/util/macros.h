#ifndef MACROS_H
#define MACROS_H

#define SECTION(name) __attribute__((section(name)))

#define INVLPG(addr) __asm__ volatile("invlpg (%0)" :: "b"(addr) : "memory")

#endif 
