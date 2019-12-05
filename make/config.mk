MAKEFLAGS += --silent

HERE=$(shell pwd)
ROOT=$(dir $(realpath $(lastword $(MAKEFILE_LIST))))../

BUILD = $(ROOT)/build/$(TARGET)
BIN = $(BUILD)/bin

CXXFLAGS = 	-ffreestanding \
			-nostdlib \
			-m64 \
			-Wextra \
			-Wall \
			-W \
			-pedantic \
			-mcmodel=kernel \
			-fno-PIC \
			-nostdinc \
			-march=native \
			-fno-exceptions \
			-fno-rtti \
			-fuse-cxa-atexit \
			-fno-threadsafe-statics \
			-mno-red-zone

LDFLAGS = -ffreestanding -nostdlib -mcmodel=kernel -fno-PIC -no-pie -lgcc

KERNEL = $(BUILD)/kernel.bin