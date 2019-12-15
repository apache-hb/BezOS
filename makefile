all: setup
	$(MAKE) ROOT=$(shell pwd) -C kernel

setup:
	@ mkdir -p build/kernel/bin

clean:
	@ rm -rf build/kernel