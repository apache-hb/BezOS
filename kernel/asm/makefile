include $(ROOT)/scripts/common.make

all:
	$(CL_ASM) multiboot.asm -o $(BIN)/kernel_asm_multiboot.asm.o
	$(CC) -mcmodel=kernel -fno-use-linker-plugin -nostdlib -ffreestanding -T $(HERE)/link.ld $(BIN)/*.o -o $(BIN)/kernel.elf
	$(COPY) -O elf32-i386 $(BIN)/kernel.elf $(ROOT)/build/kernel/kernel.bin