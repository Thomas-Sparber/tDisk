/**
  *
  * tDisk Driver
  * @author Thomas Sparber (2015-2016)
  *
  * This file contains configuration options for the tDisk driver
  *
 **/

#ifndef CONFIG_H
#define CONFIG_H

/**
  * Defines whether the kernel should use netlink
  * The entire plugin support is dependent on netlink
 **/
#define USE_NETLINK

/**
  * Defines whether plugin support should be enabled
  * Netlink support is required for this
 **/
#ifdef USE_NETLINK
#define USE_PLUGINS
#endif //USE_NETLINK

/**
  * Defines whether the driver should be able to use
  * files or block devices as internal devices
 **/
#define USE_FILES

/**
  * Defines whether automatic sector movement should be enabled
 **/
#define MOVE_SECTORS

/**
  * Defines whether automatic reset of sector access count should
  * be enabled. This prevents variable overflows
 **/
#define AUTO_RESET_ACCESS_COUNT

/**
  * Defines whether the performance of the devices should be measured
 **/
#define MEASURE_PERFORMANCE

#endif //CONFIG_H
