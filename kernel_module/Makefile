obj-m := tdisk.o
ccflags-y := -I$(PWD)/../include
tdisk-objs := \
	src/tdisk.o \
	src/tdisk_control.o \
	src/tdisk_nl.o \
	src/worker_timeout.o

module:
	@$(MAKE) -C /lib/modules/$(shell uname -r)/build SUBDIRS=$(shell pwd) modules

clean:
	@$(MAKE) -C /lib/modules/$(shell uname -r)/build SUBDIRS=$(shell pwd) clean

