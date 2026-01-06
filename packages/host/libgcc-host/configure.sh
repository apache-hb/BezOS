#!/bin/sh

cd $PKGTOOL_BUILDDIR

$PKGTOOL_SYSROOT/src/gcc/configure \
    --prefix=$PKGTOOL_PREFIX \
    --disable-multilib \
    --enable-languages=c \
    --disable-bootstrap
