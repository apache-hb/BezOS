CC_X86 = x86_64-elf-g++
CC_ARM = arm-none-eabi

# get the path of the build
ROOT = $(dir $(realpath $(lastword $(MAKEFILE_LIST))))../
BUILD = $(ROOT)/build
# path to whatever includes this
HERE = $(shell pwd)

MAKEFLAGS += --silent
CXXFLAGS = 	-O2 \
			-g \
			-std=c++17 \
			-W \
			-Wall \
			-Wextra \
			-mcmodel=kernel \
			-mno-red-zone \
			-nostdinc \
			-ffreestanding \
			-march=native \
			-fno-PIC \
			-pedantic \
			-mno-sse2 \
			-mno-sse3 \
			-mno-ssse3 \
			-mno-sse4.1 \
			-mno-sse4.2 \
			-mno-sse4 \
			-mno-sse4a \
			-mno-3dnow \
			-mno-avx \
			-mno-avx2 \
			-fno-exceptions \
			-fno-rtti \
			-fuse-cxa-atexit \
			-fno-threadsafe-statics \
			-fno-builtin \
			-m64

LDFLAGS = 	-ffreestanding \
			-nostdlib \
			-lgcc \
			-mcmodel=kernel \
			-fno-PIC \
			-fno-pie \
			-Wl,--build-id=none \
			-m64 