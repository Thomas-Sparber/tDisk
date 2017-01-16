#include "internal.h"
#include "tfs.h"

const struct file_operations tfs_file_operations = {
	.read_iter	= generic_file_read_iter,
	.write_iter	= generic_file_write_iter,
	.mmap		= generic_file_mmap,
	.fsync		= noop_fsync,
	.splice_read	= generic_file_splice_read,
	.splice_write	= iter_file_splice_write,
	.llseek		= generic_file_llseek,
};

const struct inode_operations tfs_file_inode_operations = {
	.setattr	= simple_setattr,
	.getattr	= simple_getattr,
};
