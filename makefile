include etc/config.mk

.PHONY: clean _setup x86_64 armv8 help

help:
	@echo "make [ x86_64 | armv8 ]"

setup:
	@ mkdir -p $(HERE)/build/$(TARGET)/bin

x86_64:
	@$(MAKE) TARGET=x86_64 setup
	@$(MAKE) TARGET=x86_64 ROOT=$(shell pwd) -C kernel
	@$(MAKE) TARGET=x86_64 ROOT=$(shell pwd) -C arch/x86_64

armv8: setup
	@$(MAKE) TARGET=armv8 setup
	@$(MAKE) TARGET=armv8 ROOT=$(shell pwd) -C kernel
	@$(MAKE) TARGET=armv8 ROOT=$(shell pwd) -C arch/armv8

clean:
	@rm -rf build/