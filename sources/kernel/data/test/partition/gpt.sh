#!/bin/sh
DST=$1

dd if=/dev/zero of=$DST bs=8M count=512 && sync

# Find the first available loop device
LOOPDEV=$(losetup -f)

# Associate the loop device with the disk image
losetup -P $LOOPDEV $DST

# Create a partition table
parted -s $LOOPDEV mklabel gpt

# Create a partition
parted -s $LOOPDEV mkpart primary fat32 2048s 20%

parted -s $LOOPDEV name 1 efiboot

parted -s $LOOPDEV mkpart primary ext4 20% 100%

parted -s $LOOPDEV name 2 rootfs

# Format the partition
mkfs.fat ${LOOPDEV}p1

mkfs.ext4 -q ${LOOPDEV}p2

# Set the boot flag
parted -s $LOOPDEV set 1 boot on

# Detach the loop device
losetup -d $LOOPDEV
