include make/config.mk

.PHONY: help

help:
	@ echo "make [x86_64 | arm]"

setup:
	@ mkdir -p $(HERE)/build/$(TARGET)/bin

x86_64:
	@ $(MAKE) TARGET=x86_64 setup
	@ $(MAKE) TARGET=x86_64 CXX=x86_64-elf-g++ -C kernel
	@ $(MAKE) TARGET=x86_64 CXX=x86_64-elf-g++ -C arch/x86_64

arm: setup
	@ $(MAKE) TARGET=arm setup
	@ $(MAKE) TARGET=arm CXX=arm-none-eabi-g++ -C kernel
	@ $(MAKE) TARGET=arm CXX=arm-none-eabi-g++ -C arch/arm
