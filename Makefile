.PHONY: help

help:
	@echo "make [x86_64 | arm]"

setup:
	@mkdir -p $(shell pwd)/build/$(TARGET)/bin

x86_64:
	@echo "building for x86_64"
	@$(MAKE) TARGET=x86_64 setup
	@$(MAKE) TARGET=x86_64 CXX=x86_64-elf-g++ -C src

arm:
	@echo "building for arm"
	@$(MAKE) TARGET=arm setup
	@$(MAKE) TARGET=arm CXX=arm-none-eabi-g++ -C src