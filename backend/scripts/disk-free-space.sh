#!/bin/sh

if [ $# -lt 1 ]; then
	echo "get_mount_point needs the device to be looking for"
	echo "e.g. $0 /dev/sda1"
	exit 1
fi

mountpoint=$(findmnt -o TARGET --noheadings -f $1)

if [ $? -eq 0 ]; then
	avail=$(df --output=avail $1 | tail -n1)
	echo $avail
	return 0
else
	mkdir /tmp/td_mount_test
	mount $1 /tmp/td_mount_test 2>1 > /dev/null
	if [ $? -eq 0 ]; then
		avail=$(df --output=avail $1 | tail -n1)
		umount $1
		rmdir /tmp/td_mount_test
		echo $avail
		return 0
	else
		rmdir /tmp/td_mount_test
		return 1
	fi
fi
