#!/bin/bash

LOC=~/oslocal/HW/HW5/
SHARE=~/osshare/HW/HW5/

if [ "$1" == "usr" ]; then
    cp -r $SHARE $LOC
    cd $LOC
    echo "building user mode"
    gcc -o message_sender message_sender.c
elif [ "$1" == "ker" ]; then
    rm -r $LOC
    ./build.sh usr
    cd $LOC
    echo "building kernel mode"
    make clean
    make all
fi
