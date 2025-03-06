#!/bin/sh

cd $1
$PREFIX/autoconf/bin/autoreconf -i -f
cd -
