/**
  *
  * tDisk Driver
  * @author Thomas Sparber (2015)
  *
 **/

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/stat.h>
#include <linux/errno.h>
#include <linux/major.h>
#include <linux/blkdev.h>
#include <linux/miscdevice.h>
#include <linux/falloc.h>
#include <linux/uio.h>

#include "tdisk_control.h"
#include "tdisk.h"

DEFINE_MUTEX(td_index_mutex);

/**
  * This function handles the ioctl calls
  * of the td-control device
 **/
static long tdisk_control_ioctl(struct file *file, unsigned int cmd, unsigned long parm)
{
	struct tdisk *td;
	int ret = -ENOSYS;

	mutex_lock(&td_index_mutex);
	switch(cmd)
	{
	case TDISK_CTL_ADD:
		ret = tdisk_lookup(&td, parm);
		if(ret >= 0)
		{
			ret = -EEXIST;
			break;
		}
		ret = tdisk_add(&td, parm, 16384, 1024);	//TODO
		break;
	case TDISK_CTL_REMOVE:
		ret = tdisk_lookup(&td, parm);
		if(ret < 0)break;
		mutex_lock(&td->ctl_mutex);
		if(td->state != state_unbound)
		{
			ret = -EBUSY;
			mutex_unlock(&td->ctl_mutex);
			break;
		}
		if(atomic_read(&td->refcount) > 0)
		{
			ret = -EBUSY;
			mutex_unlock(&td->ctl_mutex);
			break;
		}
		td->kernel_disk->private_data = NULL;
		mutex_unlock(&td->ctl_mutex);
		tdisk_remove(td);
		break;
	case TDISK_CTL_GET_FREE:
		ret = tdisk_lookup(&td, -1);
		if(ret >= 0)break;	//Means there is already an available device
		ret = tdisk_add(&td, -1, 16384, 1024);		//TODO
	}
	mutex_unlock(&td_index_mutex);

	return ret;
}

/**
  * Represents the file operations of the td-control device
 **/
static const struct file_operations tdisk_ctl_fops = {
	.open		= nonseekable_open,
	.unlocked_ioctl	= tdisk_control_ioctl,
	.compat_ioctl	= tdisk_control_ioctl,
	.owner		= THIS_MODULE,
	.llseek		= noop_llseek,
};

/**
  * Mic device operations
 **/
static struct miscdevice tdisk_misc = {
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= "td-control",
	.fops		= &tdisk_ctl_fops,
};

MODULE_ALIAS_MISCDEV(MISC_DYNAMIC_MINOR);
MODULE_ALIAS("devname:td-control");

/**
  * Registers the td-control device
 **/
int register_tdisk_control(void)
{
	return misc_register(&tdisk_misc);
}

/**
  * Unregisters the td-control device
 **/
void unregister_tdisk_control(void)
{
	misc_deregister(&tdisk_misc);
}
