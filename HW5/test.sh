#!/bin/bash

cd ~/oslocal/HW/HW5/
sudo insmod message_slot.ko
sudo mknod /dev/simple_message_slot c 250 0
./message_sender
sudo rm -f /dev/simple_message_slot
sudo rmmod message_slot
dmesg
