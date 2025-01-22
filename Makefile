.PHONY: build
build:
	meson install -C build --quiet

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
	pwsh.exe -File data/test/vm/Test-VirtualBox.ps1 -KernelImage install/bezos.iso

.PHONY: vmware
vmware: build
	pwsh.exe -File data/test/vm/Test-VMware.ps1 -KernelImage install/bezos.iso

.PHONY: hyperv
hyperv: build
	pwsh.exe -File data/test/vm/Test-HyperV.ps1 -KernelImage install/bezos.iso

.PHONY: pxe
pxe: build
	pwsh.exe -File data/test/pxe/Copy-PxeImage.ps1 -KernelFolder install/pxe -PxeServerFolder C:/Users/elliothb/Documents/tftpserver

.PHONY: check
check:
	meson test -C build

.PHONY: clean
clean:
	ninja -C build clean

.PHONY: integration
integration: qemu vbox vmware hyperv
