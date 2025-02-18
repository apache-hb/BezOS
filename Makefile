LIMINE_ISO=install/bezos-limine.iso
HYPER_ISO=install/bezos-hyper.iso

HYPER_BOOTX64=data/hyper/BOOTX64.EFI
HYPER_ISOBOOT=data/hyper/hyper-iso-boot
HYPER_INSTALL=data/hyper/hyper-install

OVMF_CODE=install/ovmf/ovmf-code-x86_64.fd
OVMF_VARS=install/ovmf/ovmf-vars-x86_64.fd

LIMINE_CONF=install/limine/image/boot/limine.conf
HYPER_CONF=install/hyper/image/boot/hyper.conf

.DEFAULT_GOAL := install

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

$(HYPER_CONF): data/hyper.conf
	mkdir -p install/hyper/image/boot
	cp data/hyper.conf $(HYPER_CONF)

$(LIMINE_CONF): data/limine.conf
	mkdir -p install/limine/image/boot
	cp data/limine.conf $(LIMINE_CONF)

.PHONY: build
build: hyper
	meson install -C build/system --quiet
	meson install -C build/kernel --quiet

.PHONY: install
install: build $(LIMINE_CONF) $(HYPER_CONF)
	PREFIX=$(shell pwd)/install BUILD=$(shell pwd)/build ./data/image.sh

.PHONY: qemu
qemu: install
	./qemu.sh

.PHONY: qemu-ovmf
qemu-ovmf: install-ovmf
	./qemu.sh ovmf

.PHONY: vbox
vbox: install
	pwsh.exe -File data/test/vm/Test-VirtualBox.ps1 -KernelImage $(LIMINE_ISO)

.PHONY: vmware
vmware: install
	pwsh.exe -File data/test/vm/Test-VMware.ps1 -KernelImage $(LIMINE_ISO)

.PHONY: hyperv
hyperv: install
	pwsh.exe -File data/test/vm/Test-HyperV.ps1 -KernelImage $(LIMINE_ISO)

.PHONY: pxe
pxe: install
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
