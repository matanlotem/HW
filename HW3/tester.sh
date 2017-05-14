#!/bin/bash

awk 'BEGIN{FS="c"; R=0;}{R=R+NF-1;}END{print R;}' ./dispatcher.c
awk 'BEGIN{FS="c"; R=0;}{R=R+NF-1;}END{print R;}' ./man.txt
