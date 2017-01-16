#pragma GCC system_header
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/highmem.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/backing-dev.h>
#include <linux/pagevec.h>
#include <linux/mman.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

extern const struct inode_operations tfs_file_inode_operations;
