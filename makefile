.PHONY: clean _setup x86_64 armv8 help

help:
	@echo "make [ x86_64 | armv8 ]"

BUILDDIR = build/bin/

$(BUILDDIR):
	@mkdir -p $(BUILDDIR)

x86_64: $(BUILDDIR)
	@$(MAKE) ROOT=$(shell pwd) -C arch/x86_64
	@$(MAKE) ROOT=$(shell pwd) -C kernel

armv8: $(BUILDDIR)
	@$(MAKE) ROOT=$(shell pwd) -C arch/armv8
	@$(MAKE) ROOT=$(shell pwd) -C kernel

clean:
	@rm -rf build/