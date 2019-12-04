include ../../../make/cc.mk

CFLAGS += -mcmodel=kernel -mno-red-zone -ffreestanding

all: $(HERE)/boot.s $(HERE)/link.ld
	$(CC_X86) $(HERE)/boot.s -MD -MP -c -o $(BUILD)/boot.o
	$(CC_X86) $(CXXFLAGS) -c $(ROOT)/src/kernel.c -o $(BUILD)/kernel.o
	$(CC_X86) -T $(HERE)/link.ld -ffreestanding -O2 -nostdlib $(BUILD)/kernel.o $(BUILD)/boot.o -o $(BUILD)/bezos.bin -lgcc