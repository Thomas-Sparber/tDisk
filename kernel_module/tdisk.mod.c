#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x332aef30, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x57fa1608, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0x12da5bb2, __VMLINUX_SYMBOL_STR(__kmalloc) },
	{ 0xbed7f1bb, __VMLINUX_SYMBOL_STR(alloc_disk) },
	{ 0xfbc74f64, __VMLINUX_SYMBOL_STR(__copy_from_user) },
	{ 0x77c0ecc, __VMLINUX_SYMBOL_STR(blk_cleanup_queue) },
	{ 0xe1b190b3, __VMLINUX_SYMBOL_STR(bio_alloc_bioset) },
	{ 0x616fc5f3, __VMLINUX_SYMBOL_STR(mem_map) },
	{ 0xd6ee688f, __VMLINUX_SYMBOL_STR(vmalloc) },
	{ 0x67c2fa54, __VMLINUX_SYMBOL_STR(__copy_to_user) },
	{ 0x2e5810c6, __VMLINUX_SYMBOL_STR(__aeabi_unwind_cpp_pr1) },
	{ 0x43a53735, __VMLINUX_SYMBOL_STR(__alloc_workqueue_key) },
	{ 0x107e1c8d, __VMLINUX_SYMBOL_STR(blk_mq_start_request) },
	{ 0x8e196966, __VMLINUX_SYMBOL_STR(blk_mq_unfreeze_queue) },
	{ 0x82dcc016, __VMLINUX_SYMBOL_STR(kobject_uevent) },
	{ 0xf7802486, __VMLINUX_SYMBOL_STR(__aeabi_uidivmod) },
	{ 0xb1ad28e0, __VMLINUX_SYMBOL_STR(__gnu_mcount_nc) },
	{ 0x62b72b0d, __VMLINUX_SYMBOL_STR(mutex_unlock) },
	{ 0x6b9835f5, __VMLINUX_SYMBOL_STR(vfs_fsync) },
	{ 0x999e8297, __VMLINUX_SYMBOL_STR(vfree) },
	{ 0x7295b93a, __VMLINUX_SYMBOL_STR(blk_mq_freeze_queue) },
	{ 0x4dfaa0e9, __VMLINUX_SYMBOL_STR(blk_mq_complete_request) },
	{ 0x4fd916d2, __VMLINUX_SYMBOL_STR(blk_queue_flush) },
	{ 0x8e087afb, __VMLINUX_SYMBOL_STR(idr_for_each) },
	{ 0x91715312, __VMLINUX_SYMBOL_STR(sprintf) },
	{ 0xd697e69a, __VMLINUX_SYMBOL_STR(trace_hardirqs_on) },
	{ 0x5f1a8e31, __VMLINUX_SYMBOL_STR(nonseekable_open) },
	{ 0x6345e7d9, __VMLINUX_SYMBOL_STR(invalidate_bdev) },
	{ 0x641b090b, __VMLINUX_SYMBOL_STR(blk_mq_init_queue) },
	{ 0xe707d823, __VMLINUX_SYMBOL_STR(__aeabi_uidiv) },
	{ 0xa1eb6144, __VMLINUX_SYMBOL_STR(vfs_iter_write) },
	{ 0x31e68462, __VMLINUX_SYMBOL_STR(misc_register) },
	{ 0x29002d53, __VMLINUX_SYMBOL_STR(iov_iter_bvec) },
	{ 0xfa2a45e, __VMLINUX_SYMBOL_STR(__memzero) },
	{ 0xb615e3b1, __VMLINUX_SYMBOL_STR(set_device_ro) },
	{ 0x6756906f, __VMLINUX_SYMBOL_STR(idr_destroy) },
	{ 0xdc798d37, __VMLINUX_SYMBOL_STR(__mutex_init) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0x23afbe7b, __VMLINUX_SYMBOL_STR(del_gendisk) },
	{ 0x21fc6671, __VMLINUX_SYMBOL_STR(blk_mq_free_tag_set) },
	{ 0xc34edfb4, __VMLINUX_SYMBOL_STR(bio_add_page) },
	{ 0xa1c76e0a, __VMLINUX_SYMBOL_STR(_cond_resched) },
	{ 0xe16b893b, __VMLINUX_SYMBOL_STR(mutex_lock) },
	{ 0x8c03d20c, __VMLINUX_SYMBOL_STR(destroy_workqueue) },
	{ 0x71a50dbc, __VMLINUX_SYMBOL_STR(register_blkdev) },
	{ 0x958e28d9, __VMLINUX_SYMBOL_STR(noop_llseek) },
	{ 0x8bfd8946, __VMLINUX_SYMBOL_STR(idr_alloc) },
	{ 0x43b0c9c3, __VMLINUX_SYMBOL_STR(preempt_schedule) },
	{ 0x7e24f98b, __VMLINUX_SYMBOL_STR(fput) },
	{ 0x208af134, __VMLINUX_SYMBOL_STR(bio_put) },
	{ 0x4c0745bf, __VMLINUX_SYMBOL_STR(idr_remove) },
	{ 0xb5a459dc, __VMLINUX_SYMBOL_STR(unregister_blkdev) },
	{ 0xd8ca0e45, __VMLINUX_SYMBOL_STR(module_put) },
	{ 0xc6cbbc89, __VMLINUX_SYMBOL_STR(capable) },
	{ 0x8fc796ea, __VMLINUX_SYMBOL_STR(idr_find_slowpath) },
	{ 0xb92df223, __VMLINUX_SYMBOL_STR(bd_set_size) },
	{ 0x2fd85baf, __VMLINUX_SYMBOL_STR(___ratelimit) },
	{ 0x634d34aa, __VMLINUX_SYMBOL_STR(vfs_iter_read) },
	{ 0x3ded8798, __VMLINUX_SYMBOL_STR(put_disk) },
	{ 0xd597e7eb, __VMLINUX_SYMBOL_STR(bdgrab) },
	{ 0x6a01d063, __VMLINUX_SYMBOL_STR(blk_mq_alloc_tag_set) },
	{ 0x5c000ff0, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0x5bb5c37e, __VMLINUX_SYMBOL_STR(__module_get) },
	{ 0x1e047854, __VMLINUX_SYMBOL_STR(warn_slowpath_fmt) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0x9d669763, __VMLINUX_SYMBOL_STR(memcpy) },
	{ 0xde03a168, __VMLINUX_SYMBOL_STR(__sb_end_write) },
	{ 0x1a867faa, __VMLINUX_SYMBOL_STR(add_disk) },
	{ 0x76313248, __VMLINUX_SYMBOL_STR(fget) },
	{ 0x9ade4f3b, __VMLINUX_SYMBOL_STR(__sb_start_write) },
	{ 0x130e2fce, __VMLINUX_SYMBOL_STR(ioctl_by_bdev) },
	{ 0xb2d48a2e, __VMLINUX_SYMBOL_STR(queue_work_on) },
	{ 0x1e2cb56e, __VMLINUX_SYMBOL_STR(set_blocksize) },
	{ 0x30eaabe1, __VMLINUX_SYMBOL_STR(zero_fill_bio) },
	{ 0x4c71df2b, __VMLINUX_SYMBOL_STR(vfs_getattr) },
	{ 0xec3d2e1b, __VMLINUX_SYMBOL_STR(trace_hardirqs_off) },
	{ 0xb9e53f90, __VMLINUX_SYMBOL_STR(blk_mq_map_queue) },
	{ 0xcf5fa251, __VMLINUX_SYMBOL_STR(misc_deregister) },
	{ 0x9d9f42d9, __VMLINUX_SYMBOL_STR(bdput) },
	{ 0x390fd84b, __VMLINUX_SYMBOL_STR(flush_dcache_page) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "DD65647F6DEC2B5E2C6047F");
