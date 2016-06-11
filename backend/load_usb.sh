#!/bin/sh

if [ $(whoami) != "root" ]; then
	echo "You must be root to execute this script"
	exit 1
fi

#Load gadget driver
modprobe libcomposite

#Create gadget directory
cd /sys/kernel/config/usb_gadget/
mkdir -p tdisk
cd tdisk

#Set hardware strings
echo 0x1d6b > idVendor # Linux Foundation
echo 0x0104 > idProduct # Multifunction Composite Gadget
echo 0x0100 > bcdDevice # v1.0.0
echo 0x0200 > bcdUSB # USB2
mkdir -p strings/0x409
echo "fedcba9876543210" > strings/0x409/serialnumber
echo "tDisk" > strings/0x409/manufacturer
echo "tDisk USB Device" > strings/0x409/product

#Set hardware configuration
mkdir -p configs/c.1/strings/0x409
echo "Config 1: ECM network" > configs/c.1/strings/0x409/configuration
echo 250 > configs/c.1/MaxPower

#Load serial port
mkdir -p functions/acm.usb0
ln -s functions/acm.usb0 configs/c.1/

#Load mass storage device
ls /sys/class/udc > UDC
