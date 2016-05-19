#!/bin/sh

if [ $(whoami) != "root" ]; then
	echo "You need to be root to run tdisk-post-create"
	exit 1
fi

if [ $# -ne 1 ]; then
	echo "Usage: $0 [tDisk-path]"
	exit 1
fi

if [ ! -e $1 ]; then
	echo "tDisk $1 does not exist"
	exit 1
fi

#Check if device is already mounted
mount -l | grep $1 > /dev/null
if [ $? -eq 0 ]; then
	#Device is mounted, so unmounting it
	umount $1
fi
status=$?
if [ $status -ne 0 ]; then
	echo "Error unmounting tDisk $1"
	exit $status
fi

#Create filesystem
mkfs.ext4 -q -F $1 > /dev/null
status=$?
if [ $status -ne 0 ]; then
	echo "Error creating filesystem on tDisk"
	exit $status
fi

#Create mountpoint
mountpoint=/media/tDisk/$(basename $1)
mkdir -p $mountpoint > /dev/null
status=$?
if [ $status -ne 0 ]; then
	echo "Error creating mount point"
	exit $status
fi

#Check if disk is already mounted
mount -l | grep $mountpoint > /dev/null
if [ $? -ne 0 ]; then
	#Nothing is mounted, so mounting it
	mount $1 $mountpoint
fi

#Check mount status
status=$?
if [ $status -ne 0 ]; then
	echo "Error mounting tDisk $1 to $mountpoint"
	exit $status
fi

#Create data directory
mkdir -p $mountpoint/data
chmod 777 $mountpoint/data

echo $mountpoint/data
