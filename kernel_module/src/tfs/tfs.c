#include "internal.h"
#include "tfs.h"

#define TFS_DEFAULT_MODE	0755

static const unsigned long TFS_MAGIC = 0x14141414;

static const struct super_operations tfs_ops;
static const struct inode_operations tfs_dir_inode_operations;

static const struct address_space_operations tfs_aops = {
	.readpage	= simple_readpage,
	.write_begin	= simple_write_begin,
	.write_end	= simple_write_end,
	.set_page_dirty	= __set_page_dirty_no_writeback,
};

struct inode *tfs_get_inode(struct super_block *sb,
				const struct inode *dir, umode_t mode, dev_t dev)
{
	struct inode * inode = new_inode(sb);

	if (inode) {
		inode->i_ino = get_next_ino();
		inode_init_owner(inode, dir, mode);
		inode->i_mapping->a_ops = &tfs_aops;
		mapping_set_gfp_mask(inode->i_mapping, GFP_HIGHUSER);
		mapping_set_unevictable(inode->i_mapping);
		inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME;
		switch (mode & S_IFMT) {
		default:
			init_special_inode(inode, mode, dev);
			break;
		case S_IFREG:
			inode->i_op = &tfs_file_inode_operations;
			inode->i_fop = &tfs_file_operations;
			break;
		case S_IFDIR:
			inode->i_op = &tfs_dir_inode_operations;
			inode->i_fop = &simple_dir_operations;

			/* directory inodes start off with i_nlink == 2 (for "." entry) */
			inc_nlink(inode);
			break;
		case S_IFLNK:
			inode->i_op = &page_symlink_inode_operations;
			inode_nohighmem(inode);
			break;
		}
	}
	return inode;
}

/*
 * File creation. Allocate an inode, and we're done..
 */
/* SMP-safe */
static int
tfs_mknod(struct inode *dir, struct dentry *dentry, umode_t mode, dev_t dev)
{
	struct inode * inode = tfs_get_inode(dir->i_sb, dir, mode, dev);
	int error = -ENOSPC;

	if (inode) {
		d_instantiate(dentry, inode);
		dget(dentry);	/* Extra count - pin the dentry in core */
		error = 0;
		dir->i_mtime = dir->i_ctime = CURRENT_TIME;
	}
	return error;
}

static int tfs_mkdir(struct inode * dir, struct dentry * dentry, umode_t mode)
{
	int retval = tfs_mknod(dir, dentry, mode | S_IFDIR, 0);
	if (!retval)
		inc_nlink(dir);
	return retval;
}

static int tfs_create(struct inode *dir, struct dentry *dentry, umode_t mode, bool excl)
{
	return tfs_mknod(dir, dentry, mode | S_IFREG, 0);
}

static int tfs_symlink(struct inode * dir, struct dentry *dentry, const char * symname)
{
	struct inode *inode;
	int error = -ENOSPC;

	inode = tfs_get_inode(dir->i_sb, dir, S_IFLNK|S_IRWXUGO, 0);
	if (inode) {
		int l = strlen(symname)+1;
		error = page_symlink(inode, symname, l);
		if (!error) {
			d_instantiate(dentry, inode);
			dget(dentry);
			dir->i_mtime = dir->i_ctime = CURRENT_TIME;
		} else
			iput(inode);
	}
	return error;
}

static const struct inode_operations tfs_dir_inode_operations = {
	.create		= tfs_create,
	.lookup		= simple_lookup,
	.link		= simple_link,
	.unlink		= simple_unlink,
	.symlink	= tfs_symlink,
	.mkdir		= tfs_mkdir,
	.rmdir		= simple_rmdir,
	.mknod		= tfs_mknod,
	.rename		= simple_rename,
};

static const struct super_operations tfs_ops = {
	.statfs		= simple_statfs,
	.drop_inode	= generic_delete_inode,
	.show_options	= generic_show_options,
};

struct tfs_mount_opts {
	umode_t mode;
};

enum {
	Opt_mode,
	Opt_err
};

static const match_table_t tokens = {
	{Opt_mode, "mode=%o"},
	{Opt_err, NULL}
};

struct tfs_fs_info {
	struct tfs_mount_opts mount_opts;
};

static int tfs_parse_options(char *data, struct tfs_mount_opts *opts)
{
	substring_t args[MAX_OPT_ARGS];
	int option;
	int token;
	char *p;

	opts->mode = TFS_DEFAULT_MODE;

	while ((p = strsep(&data, ",")) != NULL) {
		if (!*p)
			continue;

		token = match_token(p, tokens, args);
		switch (token) {
		case Opt_mode:
			if (match_octal(&args[0], &option))
				return -EINVAL;
			opts->mode = option & S_IALLUGO;
			break;
		/*
		 * We might like to report bad mount options here;
		 * but traditionally tfs has ignored all mount options,
		 * and as it is used as a !CONFIG_SHMEM simple substitute
		 * for tmpfs, better continue to ignore other mount options.
		 */
		}
	}

	return 0;
}

int tfs_fill_super(struct super_block *sb, void *data, int silent)
{
	struct tfs_fs_info *fsi;
	struct inode *inode;
	int err;

	save_mount_options(sb, data);

	fsi = kzalloc(sizeof(struct tfs_fs_info), GFP_KERNEL);
	sb->s_fs_info = fsi;
	if (!fsi)
		return -ENOMEM;

	err = tfs_parse_options(data, &fsi->mount_opts);
	if (err)
		return err;

	sb->s_maxbytes		= MAX_LFS_FILESIZE;
	sb->s_blocksize		= PAGE_SIZE;
	sb->s_blocksize_bits	= PAGE_SHIFT;
	sb->s_magic		= TFS_MAGIC;
	sb->s_op		= &tfs_ops;
	sb->s_time_gran		= 1;

	inode = tfs_get_inode(sb, NULL, S_IFDIR | fsi->mount_opts.mode, 0);
	sb->s_root = d_make_root(inode);
	if (!sb->s_root)
		return -ENOMEM;

	return 0;
}

struct dentry *tfs_mount(struct file_system_type *fs_type,
	int flags, const char *dev_name, void *data)
{
	return mount_nodev(fs_type, flags, data, tfs_fill_super);
}

static void tfs_kill_sb(struct super_block *sb)
{
	kfree(sb->s_fs_info);
	kill_litter_super(sb);
}

static struct file_system_type tfs_fs_type = {
	.name		= "tfs",
	.mount		= tfs_mount,
	.kill_sb	= tfs_kill_sb,
	.fs_flags	= FS_USERNS_MOUNT,
};

int __init init_tfs_fs(void)
{
	printk(KERN_DEBUG "tfs: driver loaded\n");

	return register_filesystem(&tfs_fs_type);
}
//fs_initcall(init_tfs_fs);

static void __exit exit_tfs_fs(void)
{
	printk(KERN_DEBUG "tfs: driver unloaded\n");
	unregister_filesystem(&tfs_fs_type);
}

module_init(init_tfs_fs);
module_exit(exit_tfs_fs);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Thomas Sparber");
