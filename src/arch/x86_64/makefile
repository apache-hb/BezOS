include ../../../make/config.mk

all:
	$(MAKE) boot.o
	$(MAKE) kernel.bin

boot.o:
	$(CXX) $(CXXFLAGS) -c $(HERE)/boot.S -o $(BIN)/boot.o

kernel.bin:
	$(CXX) $(CXXFLAGS) -T $(HERE)/link.ld $(BIN)/kernel.o $(BIN)/boot.o -o $(KERNEL) $(LDFLAGS)