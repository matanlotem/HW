#!/bin/bash
DEVICE_RANGE_NAME=message_slot
DEVICE_FILE_NAME=simple_message_slot
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
    sudo mknod /dev/$DEVICE_FILE_NAME c $MAJOR_NUM 0
    sudo chmod a+w /dev/$DEVICE_FILE_NAME
fi

# run message sender
if [ "$#" == 0 ] || [ "$1" == "send" ]; then
    ./message_sender 2 hello_world
fi

#clean up
if [ "$#" == 0 ] || [ "$1" == "close" ]; then
    sudo rm -f /dev/$DEVICE_FILE_NAME
    sudo rmmod $DEVICE_RANGE_NAME
fi

# print output
dmesg
