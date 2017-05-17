#!/bin/bash

#TEST_FILE=./man.txt
#TEST_CHAR=c
TEST_CHAR=$1
TEST_FILE=$2
VALUE=$(./test_count.sh $TEST_CHAR $TEST_FILE)
RESULT=$(./real_count.sh $TEST_CHAR $TEST_FILE -s)

if [ "$RESULT" == "$VALUE" ]; then
    echo "match $RESULT"
else
    echo "not match: test=$VALUE, real=$RESULT"
fi
./real_count.sh $TEST_CHAR $TEST_FILE -s > /dev/null &
sleep 0.1
ps -fa | grep "[c]ounter.*[0-9]* [0-9]*" -o
sleep 1;
#echo "done"
