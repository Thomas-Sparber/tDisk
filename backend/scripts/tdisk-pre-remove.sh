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

mount -l | grep $1 > /dev/null
if [ $? -eq 0 ]; then
	#Device is mounted, so unmounting it
	umount $1 2>&1 > /dev/null
fi
status=$?
if [ $status -ne 0 ]; then
	echo "Error unmounting tDisk $1"
	exit $status
fi
