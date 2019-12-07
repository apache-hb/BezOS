include $(ROOT)/etc/config.mk

all:
	@ $(CXX) $(CXXFLAGS) -c $(HERE)/kernel.cpp -o $(BIN)/kernel.o
	@ $(MAKE) ROOT=$(ROOT) CXX=$(CXX) -C $(HERE)/memory
	@ $(MAKE) ROOT=$(ROOT) CXX=$(CXX) -C $(HERE)/pci