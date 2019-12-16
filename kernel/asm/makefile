include $(ROOT)/scripts/common.make

all:
	$(CL_ASM) multiboot.asm -o $(BIN)/kernel_asm_multiboot.asm.o
	$(CC) -mcmodel=kernel -fno-use-linker-plugin -nostdlib -ffreestanding -T $(HERE)/link.ld $(BIN)/*.o -o $(BIN)/bezos.elf
	$(COPY) -O binary $(BIN)/kernel.elf $(ROOT)/build/kernel/kernel.bin