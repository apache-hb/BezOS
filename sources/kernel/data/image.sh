#!/bin/sh

# Create initrd by archiving the /system directory

INITRD=${MESON_INSTALL_PREFIX}/initrd.tar

rm ${INITRD} 2> /dev/null

tar -cf ${INITRD} -C ${MESON_INSTALL_PREFIX}/../system .

# Build limine disk image

cp ${INITRD} ${MESON_INSTALL_PREFIX}/limine/image/boot/initrd

xorriso -as mkisofs -R -r -J -b boot/limine/limine-bios-cd.bin \
    -no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
    -apm-block-size 2048 --efi-boot boot/limine/limine-uefi-cd.bin \
    -efi-boot-part --efi-boot-image --protective-msdos-label \
    ${MESON_INSTALL_PREFIX}/limine/image -o ${MESON_INSTALL_PREFIX}/bezos-limine.iso \
    > /dev/null

${MESON_BUILD_ROOT}/subprojects/limine-8.6.1-binary/limine bios-install ${MESON_INSTALL_PREFIX}/bezos-limine.iso > /dev/null

# Build hyper disk image

## Create ESP image

ESP_IMAGE=${MESON_INSTALL_PREFIX}/hyper/image/esp.fat32

rm ${ESP_IMAGE}

dd if=/dev/zero of=${ESP_IMAGE} bs=1M count=2

parted -s ${ESP_IMAGE} mklabel msdos

parted -s ${ESP_IMAGE} mkpart primary fat16 2048s 100%

parted -s ${ESP_IMAGE} set 1 boot on

${MESON_SOURCE_ROOT}/data/hyper/hyper-install ${ESP_IMAGE}

## Create ISO hybrid image

cp ${INITRD} ${MESON_INSTALL_PREFIX}/hyper/image/boot/initrd

xorriso -as mkisofs -R -r -J -b boot/hyper-iso-boot \
    -no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
    -apm-block-size 2048 --efi-boot esp.fat32 \
    -efi-boot-part --efi-boot-image --protective-msdos-label \
    ${MESON_INSTALL_PREFIX}/hyper/image -o ${MESON_INSTALL_PREFIX}/bezos-hyper.iso

${MESON_SOURCE_ROOT}/data/hyper/hyper-install ${MESON_INSTALL_PREFIX}/bezos-hyper.iso

# Copy kernel to pxe directory

cp ${MESON_INSTALL_PREFIX}/limine/image/boot/bezos-limine ${MESON_INSTALL_PREFIX}/pxe/bezos-limine
