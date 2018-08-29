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

/*
 * Fill in the superblock
 */
static int raonfs_fill_super(struct super_block *sb, void *data, int silent)
{
	return 0;
}

/*
 * Create a superblock for mounting
 */
static struct dentry *raonfs_mount(struct file_system_type *fs_type,
		int flags, const char *dev_name, void *data)
{
	return mount_bdev(fs_type, flags, dev_name, data, raonfs_fill_super);
}

/*
 * Destroy a superblock
 */
static void raonfs_kill_sb(struct super_block *sb)
{
	kill_block_super(sb);
}

static struct file_system_type raonfs_fs_type = {
	.owner		= THIS_MODULE,
	.name			= "raonfs",
	.mount		= raonfs_mount,
	.kill_sb	= raonfs_kill_sb,
	.fs_flags	= FS_REQUIRES_DEV
};
MODULE_ALIAS_FS("raonfs");

static int __init raonfs_init(void)
{
	int ret;

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

	ret = register_filesystem(&raonfs_fs_type);
	if (ret) {
		raonfs_error("Failed to register filesystem\n");
		goto error_register;
	}

	return 0;

error_register:
	kmem_cache_destroy(raonfs_inode_cachep);

	return ret;
}

static void __exit raonfs_cleanup(void)
{
	printk(KERN_INFO "RAONFS: cleanup...\n");

	unregister_filesystem(&raonfs_fs_type);

	rcu_barrier();
	kmem_cache_destroy(raonfs_inode_cachep);
}

module_init(raonfs_init);
module_exit(raonfs_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Inhyeok Kim");
MODULE_DESCRIPTION("Read-Only Filesystem");
