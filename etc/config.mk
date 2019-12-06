#MAKEFLAGS += --silent

HERE=$(shell pwd)

BUILD = $(ROOT)/build/$(TARGET)
BIN = $(BUILD)/bin

#magical C++ flags
CXXFLAGS = 	-O3 \
			-std=c++17 \
			-fno-PIC \
			-mno-mmx \
			-mno-sse \
			-mno-sse2 \
			-march=native \
			-ffreestanding \
			-nostdlib \
			-mcmodel=kernel \
			-mno-red-zone \
			-fno-exceptions \
			-fno-rtti \
			-I$(ROOT)

LDFLAGS = -nostdlib -lgcc -no-pie -zmax-page-size=0x1000

KERNEL = $(BUILD)/kernel.bin