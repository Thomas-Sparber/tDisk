#!/bin/bash

for number in {0..100000}
do
	sudo dd if=/dev/zero of=/media/tDisk/td0/data/testfile bs=512 count=1
	echo $number
	echo 3 | sudo tee /proc/sys/vm/drop_caches
done
