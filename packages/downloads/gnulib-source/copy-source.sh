#!/bin/sh

echo "Copying gnulib source from $PKGTOOL_CACHEDIR to $PKGTOOL_PREFIX/src/gnulib"
rsync --mkpath -a $PKGTOOL_CACHEDIR/ $PKGTOOL_PREFIX/src/gnulib
