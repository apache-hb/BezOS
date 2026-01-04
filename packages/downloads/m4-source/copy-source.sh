#!/bin/sh

set -ex

echo "Copying m4 source from $PKGTOOL_CACHEDIR to $PKGTOOL_PREFIX/src/m4"
rsync --mkpath -a $PKGTOOL_CACHEDIR/ $PKGTOOL_PREFIX/src/m4

cd $PKGTOOL_PREFIX/src/m4

PATH=$PKGTOOL_SYSROOT/bin:$PATH AM_CFLAGS="" AM_CXXFLAGS="" $PKGTOOL_PREFIX/src/m4/bootstrap \
    --gnulib-srcdir=$PKGTOOL_SYSROOT/src/gnulib \
    --skip-git \
    --skip-po
