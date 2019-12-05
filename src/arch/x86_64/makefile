include ../../../make/cc.mk

LDFLAGS += -Wl,-z,max-page-size=0x1000,-n,-T,$(HERE)/link.ld

all: $(HERE)/boot.S $(HERE)/link.ld
	$(CC_X86) $(HERE)/boot.S -MD -MP -c -o $(BUILD)/boot.o
	$(CC_X86) $(CXXFLAGS) -c $(ROOT)/src/kernel.cpp -o $(BUILD)/kernel.o
	$(CC_X86) $(CXXFLAGS) $(LDFLAGS) $(BUILD)/kernel.o $(BUILD)/boot.o -o $(BUILD)/bezos.bin
