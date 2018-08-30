#ifndef __RAONFS_H__
#define __RAONFS_H__

#include <linux/types.h>
#include <linux/fs.h>

#define RAONFS_MAGIC	0x4e4f4152

/*
 * Superblock structure on disk of raonfs
 */
struct raonfs_superblock {
	__le32 magic;
	__le32 textbase;
	__le32 textsize;
	__le32 fsname;
	__le32 fssize;
	__le32 blocksize;
	__le32 ioffset;
};

/*
 * Inode structure on disk of raonfs
 */
struct raonfs_inode {
	__le32 size;
	__le32 mode;
	__le32 link;
	__le64 mtime;
	__le64 ctime;
	__le64 doffset;
};

/*
 * Directory entry structure on disk of raonfs
 */
struct raonfs_dentry {
	__le32 name;
	__le32 ioffset;
};

/*
 * Inode data in memory of raonfs
 */
struct raonfs_inode_info {
	struct inode vfs_inode;

	__u32 size;
	__u32 mode;
	__u32 link;
	__u64 doffset;
};

/*
 * Superblock data in memory of raonfs
 */
struct raonfs_sb_info {
	__u32 textbase;
	__u32 textsize;
	__u32 fsname;
	__u32 fssize;
	__u32 ioffset;
};

static inline struct raonfs_inode_info *RAONFS_INODE(struct inode *inode)
{
	return container_of(inode, struct raonfs_inode_info, vfs_inode);
}

static inline struct raonfs_sb_info *RAONFS_SB(struct super_block *sb)
{
	return sb->s_fs_info;
}

#endif
