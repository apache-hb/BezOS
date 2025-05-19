#!/bin/sh

SRC=$1
EXE=$2

cat $SRC | while read line
do
    LINE=$(llvm-addr2line -e $EXE $(echo $line | awk '{print $1}'))
    echo $LINE $(echo $line | awk '{print $2}')
done
