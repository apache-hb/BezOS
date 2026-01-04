#!/bin/sh

cd $PKGTOOL_BUILDDIR

$PKGTOOL_SYSROOT/src/binutils/configure \
    --prefix=$PKGTOOL_PREFIX
