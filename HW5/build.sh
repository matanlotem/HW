#!/bin/bash

LOC=~/oslocal/HW/HW5/
SHARE=~/osshare/HW/HW5/

if [ "$1" == "usr" ]; then
    echo "building user mode"
    cd $LOC
    cp $SHARE/*.c $LOC/
    cp $SHARE/*.h $LOC/
    gcc -o message_sender message_sender.c
    gcc -o message_reader message_reader.c
elif [ "$1" == "ker" ]; then
    echo "building kernel mode"
    rm -r $LOC
    cp -r $SHARE $LOC
    cd $LOC
    make clean
    make all
    ./build.sh usr
fi
