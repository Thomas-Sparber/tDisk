#!/bin/sh

if [ $# -lt 1 ]; then
	echo "get_mount_point needs the device to be looking for"
	echo "e.g. $0 /dev/sda1"
	exit 1
fi

mountpoint=$(findmnt -o TARGET --noheadings -f $1)

if [ $? -eq 0 ]; then
	echo $mountpoint
	return 0
else
	return 1
fi
