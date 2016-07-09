#!/bin/sh

if [ $(whoami) != "root" ]; then
	echo "You must be root to execute this script"
	exit 1
fi

#Create gadget directory
cd /sys/kernel/config/usb_gadget/
cd tdisk

#Unload mass storage device
echo "" > UDC

#Remove functions
rm configs/c.1/acm.usb0

#Remove strings
rmdir configs/c.1/strings/0x409

#Remove configuration
rmdir configs/c.1

#Remove functions
rmdir functions/acm.usb0

#Remove strings
rmdir strings/0x409

#remove gadget
cd ..
rmdir tdisk
