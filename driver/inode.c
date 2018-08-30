#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include "raonfs.h"
#include "dbmisc.h"
#include "iomisc.h"

static const unsigned char raonfs_filetype_table[] = {
	DT_UNKNOWN, DT_DIR, DT_REG, DT_LNK, DT_BLK, DT_CHR, DT_FIFO, DT_SOCK
};

/*
 * Look up an entry in a directory
 */
static struct dentry *raonfs_lookup(struct inode *dir, struct dentry *dentry, unsigned int flags)
{
	struct raonfs_sb_info *sbi = RAONFS_SB(dir->i_sb);
	struct raonfs_inode_info *ri = RAONFS_INODE(dir);
	struct raonfs_dentry rde;
	const char *dname;
	int dlen;
	int entries;
	int top, btm, idx;
	int ret;

	dname = dentry->d_name.name;
	dlen = dentry->d_name.len;

	entries = dir->i_size / sizeof(struct raonfs_dentry);

	top = 0;
	btm = entries;
	idx = entries / 2;

	while (btm - top > 0) {
		ret = raonfs_block_read(dir->i_sb, ri->doffset + idx * sizeof(struct raonfs_dentry), &rde, sizeof(struct raonfs_dentry));
		if (ret < 0)
			goto err1;
	}

	return NULL;

err1:
	return ERR_PTR(ret);
}

const struct inode_operations raonfs_dir_inode_operations = {
	.lookup		= raonfs_lookup
};

/*
 * Read the entries from a directory
 */
static int raonfs_readdir(struct file *file, struct dir_context *ctx)
{
	struct inode *dir = file_inode(file);
	struct raonfs_sb_info *sbi = RAONFS_SB(dir->i_sb);
	struct raonfs_inode_info *ri = RAONFS_INODE(dir);
	struct raonfs_dentry rde;
	char dname[512];
	int off;
	int ret;

	for (off = ctx->pos; off < dir->i_size; off += sizeof(struct raonfs_dentry)) {
		ctx->pos = off;

		ret = raonfs_block_read(dir->i_sb, ri->doffset + off, &rde, sizeof(struct raonfs_dentry));
		if (ret < 0)
			break;

		ret = raonfs_block_read(dir->i_sb, sbi->textbase + rde.nameoff, dname, rde.namelen);
		if (ret < 0)
			break;

		dname[rde.namelen] = '\0';

		if (!dir_emit(ctx, dname, rde.namelen, rde.ioffset, raonfs_filetype_table[rde.type]))
			break;
	}

	return 0;
}

const struct file_operations raonfs_dir_operations = {
	.read						= generic_read_dir,
	.iterate_shared	= raonfs_readdir,
	.llseek					= generic_file_llseek
};
