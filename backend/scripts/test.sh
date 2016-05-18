#!/bin/bash

if [ $# -lt 1 ]; then
	echo "The test program needs the amount of test lines as argument"
	echo "e.g. $0 5"
	exit 1
fi

for counter in $(seq 1 $1); do
	echo "Test"
	echo $counter
	echo "true"
	echo ""
done
