#!/bin/bash

TEST_CHAR=$1
TEST_FILE=$2
DISP_OUTPUT="/dev/tty"
if [ $# == 3 ]; then
    if [ $3 == "-s" ]; then
        DISP_OUTPUT=""
    fi
fi

./dispatcher $TEST_CHAR $TEST_FILE | tee $DISP_OUTPUT | grep [0-9]* -o -m 1
