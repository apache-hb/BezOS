#!/bin/sh

cd $PKGTOOL_BUILDDIR

make -j$(nproc) -Otarget
