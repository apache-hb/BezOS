include $(ROOT)/scripts/info.make

all:
	$(CXX) -c $(HERE)/kernel.cpp -o $(BIN)/kernel_kernel.cpp.o
	$(MAKE) ROOT=$(ROOT) -C metal
	$(MAKE) ROOT=$(ROOT) -C asm