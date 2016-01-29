#!/bin/sh

for size in 1024 512 256 128 64 32 16 8
do
    echo "Test of $size"
    ./bench $size | tee -a results/result_1.csv
done
