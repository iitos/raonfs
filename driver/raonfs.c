#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

static int __init raonfs_init(void)
{
	printk(KERN_INFO "RAONFS: init...\n");

	return 0;
}

static void __exit raonfs_cleanup(void)
{
	printk(KERN_INFO "RAONFS: cleanup...\n");
}

module_init(raonfs_init);
module_exit(raonfs_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Inhyeok Kim");
MODULE_DESCRIPTION("Read-Only Filesystem");
