/**
  *
  * tDisk Driver
  * @author Thomas Sparber (2015-2016)
  *
 **/

#include <tdisk/config.h>
#include "tdisk_control.h"
#include "tdisk.h"

/**
  * This is just a hack in case the kernel was compiled
  * with CONFIG_DEBUG_LOCK_ALLOC. Then mutex_lock is replaced
  * with mutex_lock_nested which we can't use in a non GPL module...
  * The function is then implemented in helpers.c
 **/
#ifdef mutex_lock
#undef mutex_lock
extern void mutex_lock(struct mutex *lock);
#endif //mutex_lock

DEFINE_MUTEX(td_index_mutex);

/**
  * This function handles the ioctl calls
  * of the td-control device
 **/
static long tdisk_control_ioctl(struct file *file, unsigned int cmd, unsigned long parm)
{
	struct tdisk *td;
	struct tdisk_add_parameters params;
	int ret = -ENOSYS;

	mutex_lock(&td_index_mutex);
	switch(cmd)
	{
	case TDISK_CTL_ADD:
		if(copy_from_user(&params, (struct tdisk_add_parameters __user *)parm, sizeof(struct tdisk_add_parameters)) != 0)
		{
			ret = -EINVAL;
			break;
		}
		ret = tdisk_lookup(&td, params.minornumber);
		if(ret >= 0)
		{
			ret = -EEXIST;
			break;
		}
		ret = tdisk_add(&td, &params);
		break;
	case TDISK_CTL_REMOVE:
		ret = tdisk_lookup(&td, (int)parm);
		if(ret < 0)break;
		ret = tdisk_remove(td);
		break;
	case TDISK_CTL_GET_FREE:
		params.minornumber = -1;
		params.blocksize = (unsigned int)parm;
		ret = tdisk_add(&td, &params);
		break;
	default:
		ret = -EINVAL;
		break;
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

int register_tdisk_control(void)
{
	return misc_register(&tdisk_misc);
}

void unregister_tdisk_control(void)
{
	misc_deregister(&tdisk_misc);
}
