CC = x86_64-elf-gcc
SRC = src

setup:
	mkdir -p build

x86_64:
	make setup
	$(CC) -c $(SRC)/boot.s -o build/boot.o
	$(CC) -c $(SRC)/kernel.c -o build/kernel.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
	$(CC) -T $(SRC)/link.ld -o build/bezos.bin -ffreestanding -O2 build/boot.o build/kernel.o -nostdlib -lgcc

arm:
	make setup