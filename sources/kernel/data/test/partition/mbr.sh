#!/bin/sh
DST=$1

dd if=/dev/zero of=$DST bs=1M count=128 && sync

# Find the first available loop device
LOOPDEV=$(losetup -f)

# Associate the loop device with the disk image
losetup -P $LOOPDEV $DST

# Create a partition table
parted -s $LOOPDEV mklabel msdos

# Create the primary partition
parted -s $LOOPDEV mkpart primary ext4 2048s 100%

# Format the partition
mkfs.ext4 -q ${LOOPDEV}p1

# Set the boot flag
parted -s $LOOPDEV set 1 boot on

# Detach the loop device
losetup -d $LOOPDEV
