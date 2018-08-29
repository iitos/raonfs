#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/init.h>
#include "raonfs.h"
#include "dbmisc.h"

static struct kmem_cache *raonfs_inode_cachep;

/*
 * Initialize inode data
 */
static void raonfs_inode_init_once(void *inode)
{
	struct raonfs_inode_info *ri = inode;

	inode_init_once(&ri->vfs_inode);
}

static int __init raonfs_init(void)
{
	printk(KERN_INFO "RAONFS: init...\n");

	raonfs_inode_cachep =
		kmem_cache_create("raonfs_i",
				sizeof(struct raonfs_inode_info), 0,
				SLAB_RECLAIM_ACCOUNT | SLAB_MEM_SPREAD | SLAB_ACCOUNT,
				raonfs_inode_init_once);
	if (raonfs_inode_cachep == NULL) {
		raonfs_error("Failed to initialize inode cache\n");
		return -ENOMEM;
	}

	return 0;
}

static void __exit raonfs_cleanup(void)
{
	printk(KERN_INFO "RAONFS: cleanup...\n");

	rcu_barrier();
	kmem_cache_destroy(raonfs_inode_cachep);
}

module_init(raonfs_init);
module_exit(raonfs_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Inhyeok Kim");
MODULE_DESCRIPTION("Read-Only Filesystem");
