#!/bin/sh

cd $PKGTOOL_BUILDDIR
$PKGTOOL_SYSROOT/src/gcc/configure \
    --prefix=$PKGTOOL_PREFIX \
    --enable-languages=c,c++ \
    --disable-libstdcxx \
    --disable-multilib \
    --disable-bootstrap \
    --verbose
