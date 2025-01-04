.PHONY: build
build:
	ninja -C build install

.PHONY: check
check:
	ninja -C build test

install/ovmf/ovmf-code-x86_64.fd:
	mkdir -p install/ovmf
	wget https://github.com/osdev0/edk2-ovmf-nightly/releases/latest/download/ovmf-code-x86_64.fd -O install/ovmf/ovmf-code-x86_64.fd

install/ovmf/ovmf-vars-x86_64.fd:
	mkdir -p install/ovmf
	wget https://github.com/osdev0/edk2-ovmf-nightly/releases/latest/download/ovmf-vars-x86_64.fd -O install/ovmf/ovmf-vars-x86_64.fd

install-ovmf: install/ovmf/ovmf-code-x86_64.fd install/ovmf/ovmf-vars-x86_64.fd

.PHONY: vbox
vbox:
	pwsh.exe -File data/test/vm/Test-VirtualBox.ps1 -KernelImage install/bezos.iso

.PHONY: vmware
vmware:
	pwsh.exe -File data/test/vm/Test-VMware.ps1 -KernelImage install/bezos.iso

.PHONY: hyperv
hyperv:
	pwsh.exe -File data/test/vm/Test-HyperV.ps1 -KernelImage install/bezos.iso

.PHONY: pxe
pxe:
	pwsh.exe -File data/test/pxe/Copy-PxeImage.ps1 -KernelFolder install/pxe -PxeServerFolder C:/Users/elliothb/Documents/tftpserver
