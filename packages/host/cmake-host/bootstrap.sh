#!/bin/sh

cd $PKGTOOL_BUILDDIR

export PATH=$PKGTOOL_SYSROOT/bin:$PATH
export CFLAGS="--sysroot $PKGTOOL_SYSROOT"
export CXXFLAGS="--sysroot $PKGTOOL_SYSROOT"
export CC="$PKGTOOL_SYSROOT/bin/gcc"
export CXX="$PKGTOOL_SYSROOT/bin/g++"
$PKGTOOL_SYSROOT/src/cmake/bootstrap --generator=Ninja --prefix=$PKGTOOL_PREFIX
