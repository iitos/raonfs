#ifndef __RAONFS_H__
#define __RAONFS_H__

#include <linux/types.h>
#include <linux/fs.h>

#define RAONFS_MAGIC			0x4e4f4152
#define RAONFS_NAMELEN		32

/*
 * Inode flags
 */
enum {
	RAONFS_INODE_INLINE_DATA = 0
};

/*
 * Superblock structure on disk of raonfs
 */
struct raonfs_superblock {
	__le32 magic;
	__le32 blocksize;
	__le32 ioffset;
	__le64 fssize;
	char fsname[RAONFS_NAMELEN];
} __attribute__((__packed__));

/*
 * Inode structure on disk of raonfs
 */
struct raonfs_inode {
	__le32 size;
	__le32 msize;
	__le32 rdev;
	__le16 mode;
	__le16 uid;
	__le16 gid;
	__le32 ctime;
	__le32 mtime;
	__le32 atime;
	__le32 flags;
	__le64 doffset;
	__le64 moffset;
} __attribute__((__packed__));

/*
 * Directory entry structure on disk of raonfs
 */
struct raonfs_dentry {
	__le32 nameoff;
	__le16 namelen;
	__le16 type;
	__le32 ioffset;
} __attribute__((__packed__));

/*
 * Inode data in memory of raonfs
 */
struct raonfs_inode_info {
	struct inode vfs_inode;

	unsigned long flags;

	__u64 doffset;
	__u64 moffset;
	__u32 msize;
};

/*
 * Superblock data in memory of raonfs
 */
struct raonfs_sb_info {
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

#define RAONFS_INODE_BIT_FUNCTIONS(name, field, offset)	\
	static inline int raonfs_test_inode_##name(struct inode *inode, int bit) { return test_bit(bit + (offset), &RAONFS_INODE(inode)->field); }	\
	static inline void raonfs_set_inode_##name(struct inode *inode, int bit) { set_bit(bit + (offset), &RAONFS_INODE(inode)->field); }	\
	static inline void raonfs_clear_inode_##name(struct inode *inode, int bit) { clear_bit(bit + (offset), &RAONFS_INODE(inode)->field); }

RAONFS_INODE_BIT_FUNCTIONS(flag, flags, 0);

extern const struct inode_operations raonfs_dir_inode_operations;
extern const struct file_operations raonfs_dir_operations;
extern const struct address_space_operations raonfs_address_space_operations;

extern struct inode *raonfs_iget(struct super_block *sb, unsigned long pos);

#endif
