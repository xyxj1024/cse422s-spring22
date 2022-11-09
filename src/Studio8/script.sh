#!/bin/sh

if [ ! -f do_fork ]
then
    echo "Making do_fork."
    make do_fork
fi
if [ ! -f do_fork ]
then
    echo "No do_fork. Exiting."
    exit 1
fi
echo "size do_fork:"
size do_fork
for forks in 0 1 3 10 32 100 316 1000
do
    for heap_size in 0 1000 3162 10000 31622 100000 316228
    do
        for write_frac in 0.0 0.1 0.3 0.5 0.7 0.9 1.0
        do
            echo "time do_fork $forks $heap_size $write_frac"
            time ./do_fork $forks $heap_size $write_frac
        done
    done
done
