#!/bin/sh

set -ex

echo "Copying GCC source from $PKGTOOL_CACHEDIR to $PKGTOOL_PREFIX/src/gcc"
rsync --mkpath --links -a $PKGTOOL_CACHEDIR/ $PKGTOOL_PREFIX/src/gcc

cd $PKGTOOL_PREFIX/src/gcc

PATH=$PKGTOOL_SYSROOT/bin:$PATH $PKGTOOL_SYSROOT/bin/autoreconf -vfi
