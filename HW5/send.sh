#!/bin/bash

LOC=~/oslocal/HW/HW5/

cd $LOC
./message_sender $1 $2
dmesg
