LIMINE_ISO=install/bezos-limine.iso
HYPER_ISO=install/bezos-hyper.iso

HYPER_BOOTX64=data/hyper/BOOTX64.EFI
HYPER_ISOBOOT=data/hyper/hyper-iso-boot
HYPER_INSTALL=data/hyper/hyper-install

OVMF_CODE=install/ovmf/ovmf-code-x86_64.fd
OVMF_VARS=install/ovmf/ovmf-vars-x86_64.fd

ROOT := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

.DEFAULT_GOAL := build

$(OVMF_CODE):
	mkdir -p install/ovmf
	wget https://github.com/osdev0/edk2-ovmf-nightly/releases/latest/download/ovmf-code-x86_64.fd -O $(OVMF_CODE)

$(OVMF_VARS):
	mkdir -p install/ovmf
	wget https://github.com/osdev0/edk2-ovmf-nightly/releases/latest/download/ovmf-vars-x86_64.fd -O $(OVMF_VARS)

install-ovmf: $(OVMF_CODE) $(OVMF_VARS)

$(HYPER_BOOTX64):
	mkdir -p data/hyper
	wget https://github.com/UltraOS/Hyper/releases/download/v0.9.0/BOOTX64.EFI -O $(HYPER_BOOTX64)

$(HYPER_ISOBOOT):
	mkdir -p data/hyper
	wget https://github.com/UltraOS/Hyper/releases/download/v0.9.0/hyper_iso_boot -O $(HYPER_ISOBOOT)

$(HYPER_INSTALL):
	mkdir -p data/hyper
	wget https://github.com/UltraOS/Hyper/releases/download/v0.9.0/hyper_install-linux-x86_64 -O $(HYPER_INSTALL)
	chmod +x $(HYPER_INSTALL)

hyper: $(HYPER_BOOTX64) $(HYPER_ISOBOOT) $(HYPER_INSTALL)

.PHONY: build
build: $(HYPER_BOOTX64) $(HYPER_ISOBOOT) $(HYPER_INSTALL)
	meson install -C build/system --quiet
	meson install -C build/kernel --quiet

.PHONY: qemu
qemu: build
	./qemu.sh

.PHONY: qemu-ovmf
qemu-ovmf: install-ovmf
	./qemu.sh ovmf

.PHONY: vbox
vbox: build
	pwsh.exe -File data/test/vm/Test-VirtualBox.ps1 -KernelImage $(LIMINE_ISO)

.PHONY: vmware
vmware: build
	pwsh.exe -File data/test/vm/Test-VMware.ps1 -KernelImage $(LIMINE_ISO)

.PHONY: hyperv
hyperv: build
	pwsh.exe -File data/test/vm/Test-HyperV.ps1 -KernelImage $(LIMINE_ISO)

.PHONY: pxe
pxe: build
	pwsh.exe -File data/test/pxe/Copy-PxeImage.ps1 -KernelFolder install/pxe -PxeServerFolder C:/Users/elliothb/Documents/tftpserver

.PHONY: check
check:
	meson test -C build/kernel
	meson test -C build/system

.PHONY: coverage
coverage:
	ninja -C build/kernel coverage

.PHONY: clean
clean:
	ninja -C build/kernel clean
	ninja -C build/system clean
	rm -rf install

.PHONY: integration
integration: qemu vbox vmware hyperv



PKGTOOL := install/tool/bin/package.elf
PKGTOOL_MESON := build/tool/build.ninja
PKGTOOL_SRC := tool/package/main.cpp tool/meson.build

$(PKGTOOL_MESON): $(PKGTOOL_SRC)
	-(cd tool && meson setup $(ROOT)/build/tool --prefix $(ROOT)/install/tool) > /dev/null

$(PKGTOOL): $(PKGTOOL_MESON) $(PKGTOOL_SRC)
	meson install -C build/tool --quiet

pkgtool: $(PKGTOOL)



BUILDDIR := build
REPO_DB := repo.db
REPO := build/$(REPO_DB)

REPO_CONFIG := repo/repo.xml
REPO_SRC := repo/*.xml

.PHONY: $(REPO)
$(REPO): $(PKGTOOL) $(REPO_SRC)
	$(PKGTOOL) --config $(REPO_CONFIG) --repo $(REPO_DB) --output build --prefix install --workspace bezos.code-workspace --clangd

repo: $(REPO) $(REPO_SRC)
