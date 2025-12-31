#!/bin/sh

(cd $PKGTOOL_BUILDDIR && $PKGTOOL_SYSROOT/src/cmake/bootstrap --generator=Ninja --prefix=$PKGTOOL_PREFIX)
