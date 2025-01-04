#!/bin/sh

# Build disk image

xorriso -as mkisofs -R -r -J -b boot/limine/limine-bios-cd.bin \
    -no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
    -apm-block-size 2048 --efi-boot boot/limine/limine-uefi-cd.bin \
    -efi-boot-part --efi-boot-image --protective-msdos-label \
    ${MESON_INSTALL_PREFIX}/image -o ${MESON_INSTALL_PREFIX}/bezos.iso

${MESON_BUILD_ROOT}/subprojects/limine-8.6.1-binary/limine bios-install ${MESON_INSTALL_PREFIX}/bezos.iso

# Copy kernel to pxe directory

cp ${MESON_INSTALL_PREFIX}/image/boot/bezos ${MESON_INSTALL_PREFIX}/pxe/bezos
