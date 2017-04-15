#!/bin/bash

rm test.vlt
./vault test.vlt init 50K
./vault test.vlt add ./tmpfiles/img5_5.jpg
./vault test.vlt add ./tmpfiles/img2_10.jpg
./vault test.vlt add ./tmpfiles/img1_6.jpg
./vault test.vlt rm img5_5.jpg
./vault test.vlt rm img1_6.jpg
./vault test.vlt add ./tmpfiles/img4_9.jpg
#./vault test.vlt list
#./vault test.vlt status
#./vault test.vlt defrag
#./vault test.vlt add ./tmpfiles/img4_9.jpg

