mode=$1

if [ "$mode" = "ovmf" ]; then
    shift
    make install-ovmf
    qemu-system-x86_64 \
        -drive if=pflash,format=raw,unit=0,file=install/ovmf/ovmf-code-x86_64.fd,readonly=on \
        -drive if=pflash,format=raw,unit=1,file=install/ovmf/ovmf-vars-x86_64.fd \
        -M q35 -cdrom install/bezos.iso -serial stdio $@
    exit
else
    qemu-system-x86_64 -M q35 -cdrom install/bezos.iso -serial stdio $@
fi
