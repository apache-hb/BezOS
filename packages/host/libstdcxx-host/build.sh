#!/bin/sh

cd $PKGTOOL_BUILDDIR/build

make -j$(nproc) -Otarget
