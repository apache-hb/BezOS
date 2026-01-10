#!/bin/sh

cd $PKGTOOL_BUILDDIR

export PATH=$PKGTOOL_SYSROOT/bin:$PATH
# export CFLAGS="-static"
export CXXFLAGS="-static-libstdc++"
export CC="$PKGTOOL_SYSROOT/bin/gcc"
export CXX="$PKGTOOL_SYSROOT/bin/g++"
$PKGTOOL_SYSROOT/src/cmake/bootstrap --generator=Ninja --prefix=$PKGTOOL_PREFIX
