#!/bin/bash

LOC=~/oslocal/HW/HW5/
SHARE=~/osshare/HW/HW5/

rm -r $LOC
cp -r $SHARE $LOC
cd $LOC

if [ "$1" == "usr" ]; then
    echo "building user mode"
    gcc -o message_sender message_sender.c
elif [ "$1" == "ker" ]; then
    echo "building kernel mode"
    make all
fi
