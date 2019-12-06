all:
	i386-elf-gcc -c boot.S -o boot.o
	i386-elf-gcc -c kernel.c -o kernel.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
	i386-elf-gcc -T link.ld -o myos.bin -ffreestanding -O2 -nostdlib boot.o kernel.o -lgcc