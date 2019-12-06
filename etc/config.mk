#MAKEFLAGS += --silent

HERE=$(shell pwd)
ROOT=$(dir $(realpath $(lastword $(MAKEFILE_LIST))))../

BUILD = $(ROOT)/build/$(TARGET)
BIN = $(BUILD)/bin

#magical C++ flags
CXXFLAGS = -O3 -std=c++17 -mno-mmx -mno-sse -mno-sse2 -march=native -ffreestanding -nostdlib -no-pie -fno-PIC -mcmodel=kernel -mno-red-zone

LDFLAGS = -nostdlib -lgcc

KERNEL = $(BUILD)/kernel.bin