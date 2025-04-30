# build variables

BUILDDIR := build
PREFIX := install

# pkgtool build rules

PKGTOOL := install/tool/bin/package.elf
PKGTOOL_MESON := build/tool/build.ninja
PKGTOOL_SRC := tool/package/main.cpp tool/meson.build

$(PKGTOOL_MESON): $(PKGTOOL_SRC)
	-(cd tool && meson setup $(ROOT)/build/tool --prefix $(ROOT)/install/tool) > /dev/null

$(PKGTOOL): $(PKGTOOL_MESON) $(PKGTOOL_SRC)
	meson install -C build/tool --quiet

pkgtool: $(PKGTOOL)

# repo building with pkgtool

REPO_DB := repo.db
REPO := build/$(REPO_DB)

REPO_CONFIG := repo/repo.xml
REPO_SRC := $(strip $(shell find repo/ -type f -name "*.xml"))

.PHONY: $(REPO)
$(REPO): $(PKGTOOL) $(REPO_SRC)
	$(PKGTOOL) --config $(REPO_CONFIG) --repo $(REPO_DB) --output build --prefix install --workspace bezos.code-workspace --clangd kernel sysapi system

repo: $(REPO) $(REPO_SRC)

# common library building

COMMON_PATH := build/packages/common
COMMON_GCDA := $(strip $(shell find $(COMMON_PATH) -type f -name "*.gcda"))
COMMON_COVERAGE := $(COMMON_PATH)/meson-logs/coverage.txt

.PHONY: clean-common
clean-common:
	ninja -C $(COMMON_PATH) clean

check-common:
	ninja -C $(COMMON_PATH) test

$(COMMON_COVERAGE): $(COMMON_GCDA)
	ninja -C $(COMMON_PATH) coverage

coverage-common: | $(COMMON_COVERAGE)
	@echo "Coverage report generated at $(COMMON_COVERAGE)"

# kernel building

KERNEL_PATH := build/packages/kernel
SYSAPI_PATH := build/packages/sysapi
COVERAGE_REPORT := $(KERNEL_PATH)/meson-logs/coverage.txt

GCDA_FILES := $(strip $(shell find $(KERNEL_PATH) $(COMMON_PATH) $(SYSAPI_PATH) -type f -name "*.gcda"))

.PHONY: clean-coverage
clean-coverage:
ifneq ($(GCDA_FILES),)
	@rm -f $(GCDA_FILES)
endif

.PHONY: clean
clean:
	ninja -C $(KERNEL_PATH) clean

.PHONY: check
check:
	meson test -C $(KERNEL_PATH)

$(COVERAGE_REPORT): $(GCDA_FILES)
	ninja -C $(KERNEL_PATH) coverage

coverage: | $(COVERAGE_REPORT)
	@echo "Coverage report generated at $(COVERAGE_REPORT)"

# vm testing

LIMINE_ISO=install/image/bezos-limine.iso
HYPER_ISO=install/image/bezos-hyper.iso

OVMF_CODE=install/ovmf/ovmf-code-x86_64.fd
OVMF_VARS=install/ovmf/ovmf-vars-x86_64.fd
OVMF_REPOFILE=repo/packages/ovmf.xml

ROOT := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

.DEFAULT_GOAL := repo

$(OVMF_CODE) $(OVMF_VARS): $(OVMF_REPOFILE)
	$(PKGTOOL) --config $(REPO_CONFIG) --repo $(REPO_DB) --output build --prefix install --reinstall ovmf

install-ovmf: $(OVMF_CODE) $(OVMF_VARS)

.PHONY: qemu
qemu:
	./qemu.sh

.PHONY: qemu-ovmf
qemu-ovmf: install-ovmf
	./qemu.sh ovmf

.PHONY: vbox
vbox:
	pwsh.exe -File data/test/vm/Test-VirtualBox.ps1 -KernelImage $(LIMINE_ISO)

.PHONY: vmware
vmware:
	pwsh.exe -File data/test/vm/Test-VMware.ps1 -KernelImage $(LIMINE_ISO)

.PHONY: hyperv
hyperv:
	pwsh.exe -File data/test/vm/Test-HyperV.ps1 -KernelImage $(LIMINE_ISO)

.PHONY: pxe
pxe:
	pwsh.exe -File data/test/pxe/Copy-PxeImage.ps1 -KernelFolder install/pxe -PxeServerFolder C:/Users/elliothb/Documents/tftpserver

.PHONY: integration
integration: qemu vbox vmware hyperv
