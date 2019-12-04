CC_X86 = x86_64-elf-gcc
CC_ARM = arm-none-eabi

# get the path of the build
ROOT = $(dir $(realpath $(lastword $(MAKEFILE_LIST))))../
BUILD = $(ROOT)/build
# path to whatever includes this
HERE = $(shell pwd)

MAKEFLAGS += --silent
CFLAGS = -O2 -g -std=gnu99
LDFLAGS = -ffreestanding -nostdlib -lgcc