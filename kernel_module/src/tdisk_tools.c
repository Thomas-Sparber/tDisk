#include "tdisk_tools.h"

static int __init tdisk_tools_init(void)
{
    printk(KERN_INFO "tdisk_tools: tDisk tools loaded\n");
    return 0;
}

static void __exit tdisk_tools_exit(void)
{
    printk(KERN_INFO "tdisk_tools: tDisk tools unloaded\n");
}

module_init(tdisk_tools_init);
module_exit(tdisk_tools_exit);

MODULE_AUTHOR("Thomas Sparber <thomas@sparber.eu>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Tools for the tDisk driver");
