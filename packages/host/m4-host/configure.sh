#!/bin/sh

set -ex

cd $PKGTOOL_BUILDDIR

PATH=$PKGTOOL_SYSROOT/bin:$PATH $PKGTOOL_SYSROOT/src/m4/configure --prefix=$PKGTOOL_PREFIX
