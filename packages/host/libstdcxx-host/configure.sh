#!/bin/sh

# actual brain damage to build libstdc++
# for some godforsaken reason it can't actually build itself properly, the configure script
# depends on its own output. so you have to build a partially broken version of libstdc++
# then configure it again with its own output before you get a working version.
mkdir -p $PKGTOOL_BUILDDIR/bootstrap/build
mkdir -p $PKGTOOL_BUILDDIR/bootstrap/prefix

cd $PKGTOOL_BUILDDIR/bootstrap/build

$PKGTOOL_SYSROOT/src/gcc/libstdc++-v3/configure \
    --prefix=$PKGTOOL_BUILDDIR/bootstrap/prefix \
    --disable-multilib \
    --enable-shared \
    --enable-static

make -j$(nproc) -Otarget
make install

mkdir -p $PKGTOOL_BUILDDIR/build
cd $PKGTOOL_BUILDDIR/build

export PATH=$PKGTOOL_SYSROOT/bin:$PATH

CXXFLAGS="-isystem $PKGTOOL_BUILDDIR/bootstrap/prefix/include/c++/15.2.0/bits" $PKGTOOL_SYSROOT/src/gcc/libstdc++-v3/configure \
    --prefix=$PKGTOOL_PREFIX \
    --disable-multilib \
    --enable-shared \
    --enable-static \
    --enable-libstdcxx-threads=yes
