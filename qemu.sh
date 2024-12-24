qemu-system-x86_64 \
    -drive if=pflash,format=raw,unit=0,file=ovmf-code-x86_64.fd,readonly=on \
    -drive if=pflash,format=raw,unit=1,file=ovmf-vars-x86_64.fd \
    -M q35 -cdrom install/bezos.iso -serial stdio $@
