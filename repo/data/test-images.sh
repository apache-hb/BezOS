#!/bin/sh

INITRD=${PREFIX}/initrd/initrd.tar

# Build hyper disk image

for ktest in $(ls ${PREFIX}/kernel/bin/bezos-test-*.elf); do
    KTEST=$(basename ${ktest})
    KTEST=${KTEST%.elf}
    IMAGE=${PREFIX}/image/${KTEST}
    mkdir -p ${IMAGE}/hyper/image/boot
    mkdir -p ${IMAGE}/hyper/image/EFI/BOOT

    ## Create ESP image

    ESP_IMAGE=${IMAGE}/hyper/image/esp.fat32

    rm ${ESP_IMAGE} 2> /dev/null

    dd if=/dev/zero of=${ESP_IMAGE} bs=1M count=2

    parted -s ${ESP_IMAGE} mklabel msdos

    parted -s ${ESP_IMAGE} mkpart primary fat16 2048s 100%

    parted -s ${ESP_IMAGE} set 1 boot on

    ${PREFIX}/hyper/share/hyper_install ${ESP_IMAGE}

    ## Create ISO hybrid image

    cp ${PREFIX}/kernel/bin/${KTEST}.elf ${IMAGE}/hyper/image/boot/bezos-hyper.elf

    cp ${INITRD} ${IMAGE}/hyper/image/boot/initrd

    cp ${REPO}/data/hyper.cfg ${IMAGE}/hyper/image/hyper.cfg

    cp ${PREFIX}/hyper/share/hyper_iso_boot ${IMAGE}/hyper/image/boot/hyper-iso-boot

    cp ${PREFIX}/hyper/share/BOOTX64.EFI ${IMAGE}/hyper/image/EFI/BOOT/BOOTX64.EFI

    xorriso -as mkisofs -R -r -J -b boot/hyper-iso-boot \
        -no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
        -apm-block-size 2048 --efi-boot esp.fat32 \
        -efi-boot-part --efi-boot-image --protective-msdos-label \
        ${IMAGE}/hyper/image -o ${IMAGE}/${KTEST}.iso

    chmod +x ${PREFIX}/hyper/share/hyper_install

    ${PREFIX}/hyper/share/hyper_install ${IMAGE}/${KTEST}.iso
done
