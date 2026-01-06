#!/bin/sh

cd $PKGTOOL_BUILDDIR

make all-target-libgcc -j$(nproc) -Otarget
