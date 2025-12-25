#!/bin/sh

set -e

if [ ! -f $PKGTOOL_SYSROOT/bin/package001 ]; then
    echo "Error: package001 is not installed!"
    exit 1
fi

echo "Package 002 test successful!"

mkdir -p $PKGTOOL_PREFIX/bin
touch $PKGTOOL_PREFIX/bin/package002
echo "Package 002 installed!"
