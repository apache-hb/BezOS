include make/cc.mk

setup:
	@mkdir -p $(BUILD)

x86_64:
	$(MAKE) setup
	echo "Building for x86_64"
	$(MAKE) -C src/arch/x86_64

arm:
	$(MAKE) setup
	echo "Building for arm"
	$(MAKE) -C src/arch/arm