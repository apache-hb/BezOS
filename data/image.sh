#!/bin/sh

# Create initrd by archiving the /system directory

INITRD=${PREFIX}/initrd.tar

rm ${INITRD}

tar -cf ${INITRD} -C ${PREFIX}/system .

# Build limine disk image

cp ${INITRD} ${PREFIX}/limine/image/boot/initrd

xorriso -as mkisofs -R -r -J -b boot/limine/limine-bios-cd.bin \
    -no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
    -apm-block-size 2048 --efi-boot boot/limine/limine-uefi-cd.bin \
    -efi-boot-part --efi-boot-image --protective-msdos-label \
    ${PREFIX}/limine/image -o ${PREFIX}/bezos-limine.iso \
    > /dev/null

${BUILD}/kernel/subprojects/limine-8.6.1-binary/limine bios-install ${PREFIX}/bezos-limine.iso > /dev/null

# Build hyper disk image

## Create ESP image

ESP_IMAGE=${PREFIX}/hyper/image/esp.fat32

rm ${ESP_IMAGE}

dd if=/dev/zero of=${ESP_IMAGE} bs=1M count=2

parted -s ${ESP_IMAGE} mklabel msdos

parted -s ${ESP_IMAGE} mkpart primary fat16 2048s 100%

parted -s ${ESP_IMAGE} set 1 boot on

${MESON_SOURCE_ROOT}/data/hyper/hyper-install ${ESP_IMAGE}

## Create ISO hybrid image

cp ${INITRD} ${PREFIX}/hyper/image/boot/initrd

xorriso -as mkisofs -R -r -J -b boot/hyper-iso-boot \
    -no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
    -apm-block-size 2048 --efi-boot esp.fat32 \
    -efi-boot-part --efi-boot-image --protective-msdos-label \
    ${PREFIX}/hyper/image -o ${PREFIX}/bezos-hyper.iso

${MESON_SOURCE_ROOT}/data/hyper/hyper-install ${PREFIX}/bezos-hyper.iso

# Copy kernel to pxe directory

cp ${PREFIX}/limine/image/boot/bezos-limine ${PREFIX}/pxe/bezos-limine
