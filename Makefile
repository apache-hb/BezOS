.PHONY: build
build:
	ninja -C build install

install/ovmf/ovmf-code-x86_64.fd:
	mkdir -p install/ovmf
	wget https://github.com/osdev0/edk2-ovmf-nightly/releases/latest/download/ovmf-code-x86_64.fd -O install/ovmf/ovmf-code-x86_64.fd

install/ovmf/ovmf-vars-x86_64.fd:
	mkdir -p install/ovmf
	wget https://github.com/osdev0/edk2-ovmf-nightly/releases/latest/download/ovmf-vars-x86_64.fd -O install/ovmf/ovmf-vars-x86_64.fd

install-ovmf: install/ovmf/ovmf-code-x86_64.fd install/ovmf/ovmf-vars-x86_64.fd
