include ../make/config.mk

kernel.o:
	$(CXX) $(CXXFLAGS) -c $(HERE)/kernel.cpp -o $(BIN)/kernel.o
	$(MAKE) TARGET=$(TARGET) CXX=$(CXX) -C arch/$(TARGET)