#!/bin/sh

FOLDER=$1
DST=$2

tar -cf $DST -C $FOLDER .
