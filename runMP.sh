#!/bin/bash


BINARY=$1
NB_PE=$2


for i in $(seq 0 $((NB_PE - 1))); do
    ./${BINARY} $i &
    #sleep 0.5
done


for i in $(seq 0 $((NB_PE - 1))); do
  wait
done
