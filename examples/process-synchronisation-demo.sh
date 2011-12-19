#!/bin/bash
process-cyclic &
P1=$!
sleep 1
process-waitother &
P2=$!
sleep 1
process-watch &
P3=$!

read key
kill $P1 $P2 $P3
