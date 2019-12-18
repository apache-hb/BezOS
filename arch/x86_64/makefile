include $(ROOT)/etc/config.mk

all: kernel.bin

$(BIN)/init.o:
	@nasm -felf64 $(HERE)/boot.asm -o $(BIN)/init.o

kernel.bin: $(BIN)/init.o
	@$(CC) -T $(HERE)/link.ld $(BIN)/*.o -o $(BUILD)/kernel.bin $(LDFLAGS)
	@# lets just pretend that this 64 bit is code is actually 32 bit
	@ x86_64-elf-objcopy -O elf32-i386 $(BUILD)/kernel.bin $(BUILD)/kernel32.bin
