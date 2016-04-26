obj-m := tdisk.o
#EXTRA_CFLAGS=-I$(shell pwd)/include
ccflags-y := -I$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))/include
tdisk-objs := \
	src/tdisk.o \
	src/tdisk_control.o \
	src/tdisk_nl.o \
	src/worker_timeout.o

module:
	echo $(ccflags-y)
	@$(MAKE) -C /lib/modules/$(shell uname -r)/build SUBDIRS=$(shell pwd) $(MAKE_OPTS) modules

clean:
	@$(MAKE) -C /lib/modules/$(shell uname -r)/build SUBDIRS=$(shell pwd) clean

