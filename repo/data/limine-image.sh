#!/bin/sh

# Create initrd by archiving the /system directory

IMAGE=${PREFIX}/image
INITRD=${IMAGE}/limine/initrd.tar

mkdir -p ${IMAGE}

rm ${INITRD} 2> /dev/null

tar -cf ${INITRD} -C ${PREFIX}/system/bin .

# Build limine disk image

mkdir -p ${IMAGE}/limine/image/boot/limine
mkdir -p ${IMAGE}/limine/image/EFI/BOOT

cp ${PREFIX}/kernel/bin/bezos-limine.elf ${IMAGE}/limine/image/boot/bezos-limine.elf

cp ${INITRD} ${IMAGE}/limine/image/boot/initrd

cp ${REPO}/data/limine.conf ${IMAGE}/limine/image/boot/limine/limine.conf

cp ${PREFIX}/limine/share/limine-bios-cd.bin ${IMAGE}/limine/image/boot/limine/limine-bios-cd.bin
cp ${PREFIX}/limine/share/limine-bios.sys ${IMAGE}/limine/image/boot/limine/limine-bios.sys
cp ${PREFIX}/limine/share/limine-uefi-cd.bin ${IMAGE}/limine/image/boot/limine/limine-uefi-cd.bin
cp ${PREFIX}/limine/share/limine-bios-pxe.bin ${IMAGE}/limine/image/boot/limine/limine-bios-pxe.bin
cp ${PREFIX}/limine/share/BOOTX64.EFI ${IMAGE}/limine/image/EFI/BOOT/BOOTX64.EFI
cp ${PREFIX}/limine/share/BOOTIA32.EFI ${IMAGE}/limine/image/EFI/BOOT/BOOTIA32.EFI

xorriso -as mkisofs -R -r -J -b boot/limine/limine-bios-cd.bin \
    -no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
    -apm-block-size 2048 --efi-boot boot/limine/limine-uefi-cd.bin \
    -efi-boot-part --efi-boot-image --protective-msdos-label \
    ${IMAGE}/limine/image -o ${IMAGE}/bezos-limine.iso \
    > /dev/null

${PREFIX}/limine/bin/limine-install bios-install ${IMAGE}/bezos-limine.iso > /dev/null

# Build hyper disk image

## Create ESP image

# ESP_IMAGE=${IMAGE}/hyper/image/esp.fat32

# rm ${ESP_IMAGE}

# dd if=/dev/zero of=${ESP_IMAGE} bs=1M count=2

# parted -s ${ESP_IMAGE} mklabel msdos

# parted -s ${ESP_IMAGE} mkpart primary fat16 2048s 100%

# parted -s ${ESP_IMAGE} set 1 boot on

# ${PREFIX}/data/hyper/hyper-install ${ESP_IMAGE}

# ## Create ISO hybrid image

# cp ${INITRD} ${PREFIX}/hyper/image/boot/initrd

# xorriso -as mkisofs -R -r -J -b boot/hyper-iso-boot \
#     -no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
#     -apm-block-size 2048 --efi-boot esp.fat32 \
#     -efi-boot-part --efi-boot-image --protective-msdos-label \
#     ${PREFIX}/hyper/image -o ${PREFIX}/bezos-hyper.iso

# ${SOURCE}/data/hyper/hyper-install ${PREFIX}/bezos-hyper.iso

# # Copy kernel to pxe directory

# cp ${PREFIX}/limine/image/boot/bezos-limine ${PREFIX}/pxe/bezos-limine
