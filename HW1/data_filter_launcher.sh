#!/bin/bash

DATA_SIZE=2M
INPUT_DATA=/dev/urandom
OUTPUT_DATA=./output

data_filter $DATA_SIZE $INPUT_DATA $OUTPUT_DATA
