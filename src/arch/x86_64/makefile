include ../../../make/cc.mk

CFLAGS += -mcmodel=kernel -mno-red-zone -ffreestanding

all: $(HERE)/boot.s $(HERE)/link.ld
	$(CC_X86) $(HERE)/boot.s -MD -MP -c -o $(BUILD)/boot.o
	#$(CC_X86) $(CXXFLAGS)