#!/bin/sh

SRC=$1
DST=$2

PLAINNAME=${SRC%.*}
FILENAME=$(basename $SRC)

PWD=$(pwd)

mkdir -p $PLAINNAME
cp $SRC $PLAINNAME/$FILENAME
cd $PLAINNAME
acpixtract -am $FILENAME

mv amltables.dat $DST

cd $PWD
