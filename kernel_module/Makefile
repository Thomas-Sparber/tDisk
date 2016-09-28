obj-m := tdisk.o tdisk_tools.o
# tdisk_all_in_one.o
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
	-Waggregate-return

tdisk-objs := \
	src/tdisk.o \
	src/tdisk_control.o \
	src/tdisk_debug.o \
	src/tdisk_nl.o

tdisk_tools-objs := \
	src/helpers.o \
	src/tdisk_tools.o \
	src/worker_timeout.o

tdisk_all_in_one-objs := \
	src/helpers.o \
	src/tdisk.o \
	src/tdisk_control.o \
	src/tdisk_debug.o \
	src/tdisk_nl.o \
	src/worker_timeout.o

all: check_kernel_symbols module

$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))/tdisk.mod.o: disable_cflags

disable_cflags:
	$(eval ccflags-y=)

check_kernel_symbols:
	@echo "//Auto generated file by Makefile"				 	> src/tdisk_tools_config.h
	@echo ""													>> src/tdisk_tools_config.h
	@echo "#ifndef TDISK_TOOLS_CONFIG_H"						>> src/tdisk_tools_config.h
	@echo "#define TDISK_TOOLS_CONFIG_H"						>> src/tdisk_tools_config.h
	@echo ""													>> src/tdisk_tools_config.h
	@grep __div64_32		/proc/kallsyms > /dev/null && echo "#define __div64_32_DEFINED"		>> src/tdisk_tools_config.h || true
	@echo ""													>> src/tdisk_tools_config.h
	@echo "#endif //TDISK_TOOLS_CONFIG_H"						>> src/tdisk_tools_config.h

module:
	@$(MAKE) -C /lib/modules/$(shell uname -r)/build SUBDIRS=$(shell pwd) $(MAKE_OPTS) modules

clean:
	@$(MAKE) -C /lib/modules/$(shell uname -r)/build SUBDIRS=$(shell pwd) clean

