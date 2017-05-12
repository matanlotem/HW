#!/bin/bash

rm counter
rm dispatcher
gcc -o dispatcher dispatcher.c -lm
gcc -o counter counter.c
