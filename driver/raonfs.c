#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/statfs.h>
#include "raonfs.h"
#include "dbmisc.h"
#include "iomisc.h"

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
 * Return a spent inode to the slab cache
 */
static void raonfs_inode_callback(struct rcu_head *head)
{
	struct inode *inode = container_of(head, struct inode, i_rcu);

	kmem_cache_free(raonfs_inode_cachep, RAONFS_INODE(inode));
}

static void raonfs_destroy_inode(struct inode *inode)
{
	call_rcu(&inode->i_rcu, raonfs_inode_callback);
}

/*
 * Get filesystem statistics
 */
static int raonfs_statfs(struct dentry *dentry, struct kstatfs *buf)
{
	struct super_block *sb = dentry->d_sb;

	buf->f_type = RAONFS_MAGIC;
	buf->f_bsize = sb->s_blocksize;

	return 0;
}

/*
 * Remount as read-only
 */
static int raonfs_remount(struct super_block *sb, int *flags, char *data)
{
	sync_filesystem(sb);

	*flags |= SB_RDONLY;

	return 0;
}

/*
 * Allocate a new inode
 */
static struct inode *raonfs_alloc_inode(struct super_block *sb)
{
	struct raonfs_inode_info *ri;

	ri = kmem_cache_alloc(raonfs_inode_cachep, GFP_KERNEL);

	return ri != NULL ? &ri->vfs_inode : NULL;
}

static const struct super_operations raonfs_super_operations = {
	.alloc_inode = raonfs_alloc_inode,
	.destroy_inode = raonfs_destroy_inode,
	.statfs = raonfs_statfs,
	.remount_fs = raonfs_remount
};

/*
 * Fill in the superblock
 */
static int raonfs_fill_super(struct super_block *sb, void *data, int silent)
{
	struct raonfs_superblock *rsb;
	struct raonfs_sb_info *sbi;
	int ret;

	sbi = kzalloc(sizeof(struct raonfs_sb_info), GFP_KERNEL);
	if (sbi == NULL)
		return -ENOMEM;

	sb->s_maxbytes = 0xffffffff;
	sb->s_magic = RAONFS_MAGIC;
	sb->s_flags |= SB_RDONLY | SB_NOATIME;
	sb->s_op = &raonfs_super_operations;

	rsb = kmalloc(512, GFP_KERNEL);
	if (rsb == NULL) {
		ret = -ENOMEM;
		goto err1;
	}

	ret = raonfs_block_read(sb, 0, rsb, 512);
	if (ret < 0) {
		ret = -ENOMEM;
		goto err2;
	}

	raonfs_notice("Mounting raonfs: magic(0x%x): blocksize(%d)", rsb->magic, rsb->blocksize);

	kfree(rsb);

	return 0;

err2:
	kfree(rsb);

err1:
	kfree(sbi);

	return ret;
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

	raonfs_notice("Initialize module");

	raonfs_inode_cachep =
		kmem_cache_create("raonfs_i",
				sizeof(struct raonfs_inode_info), 0,
				SLAB_RECLAIM_ACCOUNT | SLAB_MEM_SPREAD | SLAB_ACCOUNT,
				raonfs_inode_init_once);
	if (raonfs_inode_cachep == NULL) {
		raonfs_error("Failed to initialize inode cache");
		return -ENOMEM;
	}

	ret = register_filesystem(&raonfs_fs_type);
	if (ret) {
		raonfs_error("Failed to register filesystem");
		goto error_register;
	}

	return 0;

error_register:
	kmem_cache_destroy(raonfs_inode_cachep);

	return ret;
}

static void __exit raonfs_cleanup(void)
{
	raonfs_notice("Cleanup module");

	unregister_filesystem(&raonfs_fs_type);

	rcu_barrier();
	kmem_cache_destroy(raonfs_inode_cachep);
}

module_init(raonfs_init);
module_exit(raonfs_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Inhyeok Kim");
MODULE_DESCRIPTION("Read-Only Filesystem");
