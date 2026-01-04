#!/bin/sh

rsync --mkpath -a $PKGTOOL_CACHEDIR/ $PKGTOOL_PREFIX/src/binutils
