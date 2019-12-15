include $(ROOT)/scripts/common.make

all:
	$(CL_CXX) $(HERE)/kernel.cpp -o $(BIN)/kernel_kernel.cpp.o
	$(MAKE) ROOT=$(ROOT) -C metal
	$(MAKE) ROOT=$(ROOT) -C asm