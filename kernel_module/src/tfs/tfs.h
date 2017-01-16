#ifndef TFS_H
#define TFS_H

#pragma GCC system_header
#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/highmem.h>
#include <linux/time.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/backing-dev.h>
#include <linux/sched.h>
#include <linux/parser.h>
#include <linux/magic.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

struct inode *tfs_get_inode(struct super_block *sb, const struct inode *dir,
	 umode_t mode, dev_t dev);
extern struct dentry *tfs_mount(struct file_system_type *fs_type,
	 int flags, const char *dev_name, void *data);

extern int tfs_nommu_expand_for_mapping(struct inode *inode, loff_t newsize);


/*#ifdef CONFIG_MMU
static inline int
tfs_nommu_expand_for_mapping(struct inode *inode, size_t newsize)
{
	return 0;
}
#else
extern int tfs_nommu_expand_for_mapping(struct inode *inode, size_t newsize);
#endif*/


extern const struct file_operations tfs_file_operations;
extern const struct vm_operations_struct generic_file_vm_ops;
extern int __init init_tfs_fs(void);

int tfs_fill_super(struct super_block *sb, void *data, int silent);

#endif
