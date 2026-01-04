#!/bin/sh

cd $PKGTOOL_BUILDDIR

export PATH=$PKGTOOL_SYSROOT/bin:$PATH
export LD_LIBRARY_PATH=$PKGTOOL_SYSROOT/lib:$PKGTOOL_SYSROOT/lib64:$LD_LIBRARY_PATH
export CFLAGS="-isystem $PKGTOOL_SYSROOT"
export CXXFLAGS="-isystem $PKGTOOL_SYSROOT"
export CC="$PKGTOOL_SYSROOT/bin/gcc"
export CXX="$PKGTOOL_SYSROOT/bin/g++"
$PKGTOOL_SYSROOT/src/cmake/bootstrap --generator=Ninja --prefix=$PKGTOOL_PREFIX
