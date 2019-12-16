FLAGS = -I$(ROOT)/kernel \
		-nostdlib \
		-ffreestanding \
		-c -m64 \
		-mcmodel=kernel \
		-mno-sse2 \
		-mno-red-zone \
		-march=native \
		-fno-pic \
		-fno-stack-protector \
		-fno-asynchronous-unwind-tables \
		-O2 \
		-Wall \
		-Wextra \
		-no-pie

CXX_FLAGS = $(FLAGS) -fno-rtti -fno-exceptions -std=c++17
CXX = x86_64-elf-g++
CL_CXX = $(CXX) $(CXX_FLAGS)

C_FLAGS = $(FLAGS) -std=gnu99
CC = x86_64-elf-gcc
CL_CC = $(CC) $(C_FLAGS)

ASM_FLAGS = -f elf64 -F dwarf
ASM = nasm
CL_ASM = $(ASM) $(ASM_FLAGS)

COPY = x86_64-elf-objcopy

BIN = $(ROOT)/build/kernel/bin
HERE = $(shell pwd)

MAKEFLAGS += --silent