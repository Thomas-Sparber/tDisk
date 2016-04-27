/**
  *
  * tDisk Driver
  * @author Thomas Sparber (2015-2016)
  *
 **/

#ifndef TDISK_CONTROL_H
#define TDISK_CONTROL_H

#include <tdisk/config.h>

#pragma GCC system_header
#include <linux/miscdevice.h>

/**
  * Registers the td-control device
 **/
int register_tdisk_control(void);

/**
  * Unregisters the td-control device
 **/
void unregister_tdisk_control(void);

#endif //TDISK_CONTROL_H
