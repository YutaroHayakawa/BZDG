#!/bin/sh

packet_size_list=(1024 512 256 128 64 32 16 8)
batch_num_list=(2 3 4)

for num in ${batch_num_list[@]}; do
    echo $num > /sys/module/bzdg/parameters/BZDG_BATCH_NUM
    for size in ${packet_size_list[@]}; do
        echo "Test of ${num} batch ${size} packet size"
        ./bench $size | tee -a results4/result_${num}.csv
    done
done
