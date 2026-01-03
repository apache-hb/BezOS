#!/bin/sh

set -ex

echo "Copying GCC source from $PKGTOOL_CACHEDIR to $PKGTOOL_PREFIX/src/gcc"
rsync --mkpath -a $PKGTOOL_CACHEDIR/ $PKGTOOL_PREFIX/src/gcc

# export ACLOCAL_PATH="$PKGTOOL_SYSROOT/share/aclocal:$ACLOCAL_PATH"

cd $PKGTOOL_PREFIX/src/gcc
cp $PKGTOOL_SYSROOT/share/aclocal/*.m4 .

$PKGTOOL_SYSROOT/bin/autoreconf --force --install -I $PKGTOOL_SYSROOT/share/aclocal
$PKGTOOL_SYSROOT/bin/automake --add-missing --force --copy
$PKGTOOL_SYSROOT/bin/aclocal --system-acdir=$PKGTOOL_SYSROOT/share/aclocal --install
$PKGTOOL_SYSROOT/bin/libtoolize --force --copy --automake
