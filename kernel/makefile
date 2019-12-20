include $(ROOT)/etc/config.mk

all:
	@ $(CXX) $(CFLAGS) -c $(HERE)/kernel.cpp -o $(BIN)/kernel.o
	@ $(CXX) $(CFLAGS) -c $(HERE)/modules/vga/vga.cpp -o $(BIN)/vga.o
	@ $(CXX) $(CFLAGS) -c $(HERE)/modules/memory/memory.cpp -o $(BIN)/memory.o