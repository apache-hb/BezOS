#!/bin/sh

cd $PKGTOOL_BUILDDIR
$PKGTOOL_SYSROOT/src/gcc/libstdc++-v3/configure \
    --prefix=$PKGTOOL_PREFIX \
    --disable-multilib \
    --enable-shared \
    --enable-static
