#!/bin/sh

MODE=$1

serial_chardev()
{
    echo "-chardev stdio,id=char0,logfile=$1,signal=off -serial chardev:char0"
}

if [ "$MODE" = "ovmf" ]; then
    shift
    make install-ovmf
    qemu-system-x86_64 \
        -drive if=pflash,format=raw,unit=0,file=install/ovmf/ovmf-code-x86_64.fd,readonly=on \
        -drive if=pflash,format=raw,unit=1,file=install/ovmf/ovmf-vars-x86_64.fd \
        -m 4G \
        -M q35 -cdrom install/bezos.iso $(serial_chardev ovmf-serial.txt) $@
    exit
elif [ "$MODE" = "numa" ]; then
    shift
    make install-ovmf
    qemu-system-x86_64 \
        -drive if=pflash,format=raw,unit=0,file=install/ovmf/ovmf-code-x86_64.fd,readonly=on \
        -drive if=pflash,format=raw,unit=1,file=install/ovmf/ovmf-vars-x86_64.fd \
        -object memory-backend-ram,size=1G,id=mem0 \
        -object memory-backend-ram,size=1G,id=mem1 \
        -object memory-backend-ram,size=1G,id=mem2 \
        -object memory-backend-ram,size=1G,id=mem3 \
        -smp cpus=16 \
        -numa node,cpus=0-3,nodeid=0,memdev=mem0 \
        -numa node,cpus=4-7,nodeid=1,memdev=mem1 \
        -numa node,cpus=8-11,nodeid=2,memdev=mem2 \
        -numa node,cpus=12-15,nodeid=3,memdev=mem3 \
        -numa dist,src=0,dst=1,val=15 \
        -numa dist,src=2,dst=3,val=15 \
        -numa dist,src=0,dst=2,val=20 \
        -numa dist,src=0,dst=3,val=20 \
        -numa dist,src=1,dst=2,val=20 \
        -numa dist,src=1,dst=3,val=20 \
        -m 4G \
        -M q35 -cdrom install/bezos.iso $(serial_chardev numa-serial.txt) $@
    exit
else
    qemu-system-x86_64 -M q35 -cdrom install/bezos.iso $(serial_chardev qemu-serial.txt) $@
fi
