#MAKEFLAGS += --silent

HERE=$(shell pwd)
ROOT=$(dir $(realpath $(lastword $(MAKEFILE_LIST))))../

BUILD = $(ROOT)/build/$(TARGET)
BIN = $(BUILD)/bin

#magical C++ flags
CFLAGS = 	-O3 \
			-fno-PIC \
			-mno-mmx \
			-mno-sse \
			-mno-sse2 \
			-march=native \
			-ffreestanding \
			-nostdlib \
			-mcmodel=kernel \
			-mno-red-zone \
			-fno-unwind-tables \
			-fno-asynchronous-unwind-tables \
			-fomit-frame-pointer \
			-I$(ROOT)

CXXFLAGS =  $(CFLAGS) -fno-exceptions \
			-fno-rtti \
			-std=c++17 

LDFLAGS = -nostdlib -no-pie -zmax-page-size=0x1000 -fno-use-linker-plugin

LD = x86_64-elf-ld
CC  = x86_64-elf-gcc
CXX = x86_64-elf-g++

KERNEL = $(BUILD)/kernel.bin