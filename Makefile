LIMINE_ISO=install/bezos-limine.iso
HYPER_ISO=install/bezos-hyper.iso

HYPER_BOOTX64=data/hyper/BOOTX64.EFI
HYPER_ISOBOOT=data/hyper/hyper-iso-boot
HYPER_INSTALL=data/hyper/hyper-install

.DEFAULT_GOAL := build

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

install/ovmf/ovmf-code-x86_64.fd:
	mkdir -p install/ovmf
	wget https://github.com/osdev0/edk2-ovmf-nightly/releases/latest/download/ovmf-code-x86_64.fd -O install/ovmf/ovmf-code-x86_64.fd

install/ovmf/ovmf-vars-x86_64.fd:
	mkdir -p install/ovmf
	wget https://github.com/osdev0/edk2-ovmf-nightly/releases/latest/download/ovmf-vars-x86_64.fd -O install/ovmf/ovmf-vars-x86_64.fd

install-ovmf: install/ovmf/ovmf-code-x86_64.fd install/ovmf/ovmf-vars-x86_64.fd

.PHONY: qemu
qemu: build
	./qemu.sh

.PHONY: qemu-ovmf
qemu-ovmf: install-ovmf
	./qemu.sh ovmf

.PHONY: vbox
vbox: build
	pwsh.exe -File data/test/vm/Test-VirtualBox.ps1 -KernelImage install/kernel/bezos-limine.iso

.PHONY: vmware
vmware: build
	pwsh.exe -File data/test/vm/Test-VMware.ps1 -KernelImage install/kernel/bezos-limine.iso

.PHONY: hyperv
hyperv: build
	pwsh.exe -File data/test/vm/Test-HyperV.ps1 -KernelImage install/kernel/bezos-limine.iso

.PHONY: pxe
pxe: build
	pwsh.exe -File data/test/pxe/Copy-PxeImage.ps1 -KernelFolder install/pxe -PxeServerFolder C:/Users/elliothb/Documents/tftpserver

.PHONY: check
check:
	meson test -C build/kernel

.PHONY: coverage
coverage:
	ninja -C build/kernel coverage

.PHONY: clean
clean:
	ninja -C build/kernel clean
	rm -rf install/kernel

.PHONY: integration
integration: qemu vbox vmware hyperv
