# x86-64 bootloader
1. real.asm reads in the rest of the kernel and jumps to boot.asm
2. boot.asm enables everything needed to get into protected mode then jumps to prot.asm
3. prot.asm enables everything needed for long mode then jumps to long.asm
4. long.asm sets up basic kernel state then jumps into the main kernel