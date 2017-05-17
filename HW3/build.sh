#!/bin/bash

rm counter
rm dispatcher
gcc -o dispatcher dispatcher.c
gcc -o counter counter.c
