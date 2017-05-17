#!/bin/bash

TEST_CHAR=$1
TEST_FILE=$2

cat $TEST_FILE | grep -o $TEST_CHAR | wc -l
