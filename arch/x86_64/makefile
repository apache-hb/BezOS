include $(ROOT)/etc/config.mk

all: boot.o kernel.bin

boot.o:
	@ $(CXX) $(CXXFLAGS) -c $(HERE)/boot.cpp -o $(BIN)/boot.o
	@ $(CXX) $(CXXFLAGS) -c $(HERE)/boot.S -o $(BIN)/init.o

kernel.bin:
	@ $(CXX) $(CXXFLAGS) -T $(HERE)/link.ld $(wildcard $(BIN)/*.o) -o $(BUILD)/kernel.bin $(LDFLAGS)
	@# lets just pretend that this 64 bit is code is actually 32 bit
	@ x86_64-elf-objcopy -O elf32-i386 $(BUILD)/kernel.bin $(BUILD)/kernel.bin