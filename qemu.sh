#!/bin/sh

MODE=$1
ARGS=$@

PREFIX=install/image
ISO=$PREFIX/bezos-limine.iso
CANBUS=/tmp/canbus
QEMUARGS="-M q35 -display gtk"

serial_chardev()
{
    echo "-chardev stdio,id=uart0,logfile=$1,signal=off -serial chardev:uart0"
}

serial_canbus()
{
    echo "-chardev serial,id=canbus0,path=$CANBUS -serial chardev:canbus0"
}

repobld()
{
    install/tool/bin/package.elf --config repo/repo.xml --target repo/targets/x86_64.xml --output build --prefix install --rebuild $@
}

# Check if a different iso is requested
echo $ARGS | grep -q "\-iso=hyper"
if [ $? -eq 0 ]; then
    QEMUARGS="-cdrom $PREFIX/bezos-hyper.iso"
    ARGS=$(echo $ARGS | sed s/\-iso=hyper//)
else
    QEMUARGS="$QEMUARGS -cdrom $ISO"
fi

# Check if an hdd image is requested
echo $ARGS | grep -q "\-disk"
if [ $? -eq 0 ]; then
    # if the disk image is not present, create it
    DISKIMAGE="bezos.hdd"
    if [ ! -f "$DISKIMAGE" ]; then
        qemu-img create -f raw $DISKIMAGE 1G
    fi

    QEMUARGS="$QEMUARGS -drive file=$DISKIMAGE,format=raw"

    ARGS="$(echo $ARGS | sed s/\-disk//)"
fi

# If events are requested then create the second serial port
echo $ARGS | grep -q "\-events"
if [ $? -eq 0 ]; then
    QEMUARGS="$QEMUARGS -chardev file,id=events,path=events.bin -serial chardev:events"
    ARGS="$(echo $ARGS | sed s/\-events//)"
fi

# If smp is explicitly requested use the requested number of cpus
echo $ARGS | grep -q "\-smp"
if [ $? -eq 0 ]; then
    # Get the number of cpus requested
    CPUS=$(echo $ARGS | grep -o "\-smp\s[0-9]*" | awk '{print $2}')
    ARGS="$(echo $ARGS | sed s/\-smp\s[0-9]*//)"
    QEMUARGS="$QEMUARGS -smp $CPUS"
else
    # Otherwise use the default number of cpus
    QEMUARGS="$QEMUARGS -smp 4"
fi

if [ "$MODE" = "ovmf" ]; then
    ARGS=$(echo $ARGS | sed s/ovmf//)
    repobld ovmf kernel || exit 1
    qemu-system-x86_64 \
        -drive if=pflash,format=raw,unit=0,file=install/ovmf/ovmf-code-x86_64.fd,readonly=on \
        -drive if=pflash,format=raw,unit=1,file=install/ovmf/ovmf-vars-x86_64.fd \
        -m 4G \
        $QEMUARGS $(serial_chardev ovmf-serial.txt) $ARGS
    exit
elif [ "$MODE" = "numa" ]; then
    ARGS=$(echo $ARGS | sed s/numa//)
    make install-ovmf
    qemu-system-x86_64 \
        -drive if=pflash,format=raw,unit=0,file=install/ovmf/ovmf-code-x86_64.fd,readonly=on \
        -drive if=pflash,format=raw,unit=1,file=install/ovmf/ovmf-vars-x86_64.fd \
        -object memory-backend-ram,size=1G,id=mem0 \
        -object memory-backend-ram,size=1G,id=mem1 \
        -object memory-backend-ram,size=1G,id=mem2 \
        -object memory-backend-ram,size=1G,id=mem3 \
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
        $QEMUARGS $(serial_chardev numa-serial.txt) $ARGS
    exit
elif [ "$MODE" = "test" ]; then
    ARGS=$(echo $ARGS | sed s/test//)

    make repo || exit 1

    # Startup a socat instance to create a virtual CAN bus
    # that the guest can read data from.
    socat -d -d pty,link=/tmp/canbus,raw,echo=0 pty,link=/tmp/canbus.pty,raw,echo=0 &

    qemu-system-x86_64 $QEMUARGS $(serial_chardev qemu-serial.txt) $(serial_canbus) $ARGS
else
    repobld kernel || exit 1

    set -x
    qemu-system-x86_64 $QEMUARGS $(serial_chardev qemu-serial.txt) $ARGS
fi
