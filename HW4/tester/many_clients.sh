#!/bin/bash

for i in `seq 1 $2`; do
    ./pcc_client $1 &
done

