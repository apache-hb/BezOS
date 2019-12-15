FLAGS := -I$(ROOT)/kernel -ffreestanding -c

CXX=x86_64-elf-g++

CL_CXX = $(CXX) $(FLAGS)

CC=x86_64-elf-gcc

CL_CC = $(CC) $(FLAGS)

ASM=nasm

CL_ASM = $(ASM)

BIN=$(ROOT)/build/kernel/bin
HERE=$(shell pwd)