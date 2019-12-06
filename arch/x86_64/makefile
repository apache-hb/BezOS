include $(ROOT)/etc/config.mk

all: boot.o kernel.bin

boot.o:
	@ $(CXX) $(CXXFLAGS) $(HERE)/boot.S -o $(BIN)/boot.o

kernel.bin:
	@ $(CXX) $(CXXFLAGS) -T $(HERE)/link.ld $(wildcard $(BIN)/*.o) -o $(BUILD)/kernel.bin $(LDFLAGS)