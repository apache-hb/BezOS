#MAKEFLAGS += --silent

HERE=$(shell pwd)
ROOT=$(dir $(realpath $(lastword $(MAKEFILE_LIST))))../

BUILD = $(ROOT)/build/$(TARGET)
BIN = $(BUILD)/bin

#basic C++ flags
CXXFLAGS = -O3 -std=c++17 

#weird magic flags
CXXFLAGS += -ffreestanding -nostdlib -mcmodel=kernel

LDFLAGS = 

KERNEL = $(BUILD)/kernel.bin