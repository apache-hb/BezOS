#!/bin/sh
DST=$1

dd if=/dev/zero of=$DST bs=1M count=128 && sync

# Find the first available loop device
LOOPDEV=$(losetup -f)

# Associate the loop device with the disk image
losetup $LOOPDEV $DST

# Create a partition table
parted -s $LOOPDEV mklabel gpt

# Create a partition
parted -s $LOOPDEV mkpart primary reiserfs 2048s 100%

# Format the partition
mkfs.reiserfs ${LOOPDEV}p1
