#ifndef __RAONFS_H__
#define __RAONFS_H__

/*
 * Superblock structure on disk of raonfs
 */
struct raonfs_superblock {
	__le32 magic;
	__le32 textbase;
	__le32 textsize;
	__le32 fsname;
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
}

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
	__u32 ioffset;
};

#endif
