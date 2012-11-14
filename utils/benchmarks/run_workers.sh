#!/bin/sh

for i in {1..20}
do
	jug execute jug_benchmark.py &
done
wait
