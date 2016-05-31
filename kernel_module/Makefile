obj-m := tdisk.o
ccflags-y := -O3 -I$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))/include \
	-Wall \
	-Wextra \
	-Wshadow \
	-Wformat \
	-Wuninitialized \
	-Wno-unused-parameter \
	-Winit-self \
	-Wmissing-braces \
	-Wparentheses \
	-Wsequence-point \
	-Wreturn-type \
	-Wswitch \
	-Wswitch-default \
	-Wswitch-enum \
	-Wunused \
	-Wtype-limits \
	-Wcast-qual \
	-Wcast-align \
	-Wconversion \
	-Wjump-misses-init \
	-Wsign-compare \
	-Wsign-conversion \
	-Wlogical-op \
	-Waggregate-return \
	-Wredundant-decls

tdisk-objs := \
	src/tdisk.o \
	src/tdisk_control.o \
	src/tdisk_nl.o \
	src/worker_timeout.o

all: module

$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))/tdisk.mod.o: disable_cflags

disable_cflags:
	$(eval ccflags-y=)

module:
	@$(MAKE) -C /lib/modules/$(shell uname -r)/build SUBDIRS=$(shell pwd) $(MAKE_OPTS) modules

clean:
	@$(MAKE) -C /lib/modules/$(shell uname -r)/build SUBDIRS=$(shell pwd) clean

