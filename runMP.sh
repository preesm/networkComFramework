#!/bin/bash


BINARY=$1
NB_PE=$2

echo "Start launching..."
for i in $(seq 0 $((NB_PE - 1))); do
    ./${BINARY} $i &
done
echo "Done launching"

for i in $(seq 0 $((NB_PE - 1))); do
  wait
done
