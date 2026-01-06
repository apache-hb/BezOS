#!/bin/sh

make -C $PKGTOOL_BUILDDIR -j$(nproc) -Otarget
