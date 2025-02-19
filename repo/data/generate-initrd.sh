#!/bin/sh

# Create initrd by archiving the /system/bin directory

IMAGE=${PREFIX}/initrd
INITRD=${IMAGE}/initrd.tar

mkdir -p ${IMAGE}

rm ${INITRD} 2> /dev/null

tar -cf ${INITRD} -C ${PREFIX}/system/bin .
