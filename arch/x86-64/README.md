# x86-64 bootstrap
1. get into protected mode
    * read in the rest of the kernel from disk
    * enable the a20 line
    * disable interrupts
    * load the global descriptor table
    * enable protected mode

2. get into long mode
    * check if we have cpuid
    * check if we have extended cpuid
    * check if we have long mode
    * load paging tables
    * get into long mode (32 bit compatibility submode)
    * get into true 64 bit mode

## Memory layout
| Kernel | Paging tables | Thread stacks |
|--------|---------------|---------------|
