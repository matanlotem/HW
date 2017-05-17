#!/bin/bash

TEST_FILE=./man.txt
TEST_CHAR=c
VALUE=$(cat $TEST_FILE | grep -o $TEST_CHAR | wc -l)
RESULT=$(./dispatcher $TEST_CHAR $TEST_FILE | tee /dev/tty | grep [0-9]* -o -m 1)
if [ "$RESULT" != "$VALUE" ]; then
    echo $RESULT
    echo $VALUE
fi

