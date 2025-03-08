#!/bin/sh

# Create initrd

IMAGE=${PREFIX}/initrd
INITRD=${IMAGE}/initrd.tar

# Create directories
mkdir -p ${IMAGE}/image

# Copy over init programs
cp -r ${PREFIX}/system/*.elf ${IMAGE}/image/
cp ${PREFIX}/zsh/bin/zsh ${IMAGE}/image/zsh.elf

# Delete old initrd
rm ${INITRD} 2> /dev/null

# Create new initrd
tar -cf ${INITRD} -C ${IMAGE}/image .
