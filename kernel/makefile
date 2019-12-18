all:
	x86_64-elf-gcc -fno-asynchronous-unwind-tables -ffreestanding -fno-use-linker-plugin -nostdlib -fwhole-program kernel.c -o $(ROOT)/build/bin/kernel.o
	ld -Ttext 0x200 -oformat $(ROOT)/build/bin/kernel.o -o $(ROOT)/build/bin/kernel.bin
