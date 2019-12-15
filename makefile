.PHONY: all

setup:
	@ mkdir -p build/kernel/bin

all: setup
	$(MAKE) ROOT=$(pwd) kernel

clean:
	@ rm -rf build/kernel