#!/bin/bash
DEVICE_RANGE_NAME=message_slot
LOC=~/oslocal/HW/HW5/
cd $LOC

if [ "$#" == 0 ] || [ "$1" == "init" ]; then
    # prepare
    sudo dmesg --clear
    # load module
    sudo insmod $DEVICE_RANGE_NAME.ko
    # get major
    DEV_ROW=$(cat /proc/devices | grep "[0-9]* $DEVICE_RANGE_NAME")
    MAJOR_NUM=$(echo "$DEV_ROW" | grep -o [0-9]*)
    # create device file
    sudo mknod /dev/simple_message_slot c $MAJOR_NUM 0
fi

# run message sender
if [ "$#" == 0 ] || [ "$1" == "send" ]; then
    ./message_sender
fi

#clean up
if [ "$#" == 0 ] || [ "$1" == "close" ]; then
    sudo rm -f /dev/simple_message_slot
    sudo rmmod message_slot
fi

# print output
dmesg
