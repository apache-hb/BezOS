#!/bin/sh

SRC=$1
DST=$2
PRIVATE=$3
RUNDIR=`pwd`

FILENAME=$(basename $SRC)

mkdir -p $PRIVATE
cp $SRC $PRIVATE/$FILENAME
cd $PRIVATE

acpixtract -am $FILENAME

echo "Copying to $RUNDIR/$DST"

mv amltables.dat $RUNDIR/$DST
