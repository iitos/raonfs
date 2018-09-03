#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/mpage.h>
#include <linux/buffer_head.h>
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
	struct raonfs_inode_info *ri = RAONFS_INODE(dir);
	struct raonfs_dentry rde;
	struct inode *inode = NULL;
	char cname[512];
	const char *dname;
	int top, btm, idx;
	int cmp;
	int ret;

	dname = dentry->d_name.name;

	top = 0;
	btm = (dir->i_size - ri->msize) / sizeof(struct raonfs_dentry) - 1;

	while (top <= btm) {
		idx = (top + btm) / 2;

		ret = raonfs_block_read(dir->i_sb, ri->doffset + idx * sizeof(struct raonfs_dentry), &rde, sizeof(struct raonfs_dentry));
		if (ret < 0)
			goto err1;

		ret = raonfs_block_strcpy(dir->i_sb, ri->moffset + rde.nameoff, cname, rde.namelen);
		if (ret < 0)
			goto err1;

		cmp = strcmp(cname, dname);
		if (cmp == 0) {
			inode = raonfs_iget(dir->i_sb, rde.ioffset);
			break;
		} else if (cmp > 0) {
			btm = idx - 1;
		} else if (cmp < 0) {
			top = idx + 1;
		}
	}

	return d_splice_alias(inode, dentry);

err1:
	return ERR_PTR(ret);
}

const struct inode_operations raonfs_dir_inode_operations = {
	.lookup = raonfs_lookup
};

/*
 * Read the entries from a directory
 */
static int raonfs_readdir(struct file *file, struct dir_context *ctx)
{
	struct inode *dir = file_inode(file);
	struct raonfs_inode_info *ri = RAONFS_INODE(dir);
	struct raonfs_dentry rde;
	char dname[512];
	int off;
	int ret;

	off = ctx->pos;

	while (off < (dir->i_size - ri->msize)) {
		ret = raonfs_block_read(dir->i_sb, ri->doffset + off, &rde, sizeof(struct raonfs_dentry));
		if (ret < 0)
			break;

		ret = raonfs_block_strcpy(dir->i_sb, ri->moffset + rde.nameoff, dname, rde.namelen);
		if (ret < 0)
			break;

		if (!dir_emit(ctx, dname, rde.namelen, rde.ioffset, raonfs_filetype_table[rde.type]))
			break;

		off += sizeof(struct raonfs_dentry);
	}

	ctx->pos = off;

	return 0;
}

const struct file_operations raonfs_dir_operations = {
	.read = generic_read_dir,
	.iterate_shared = raonfs_readdir,
	.llseek = generic_file_llseek
};

/*
 * Map a block address to a bufferhead
 */
static int raonfs_get_block(struct inode *inode, sector_t iblock,
		struct buffer_head *bh, int create)
{
	struct raonfs_inode_info *ri = RAONFS_INODE(inode);

	if (create) {
		raonfs_error("can't allocate new block");
		return -EROFS;
	}

	map_bh(bh, inode->i_sb, (ri->doffset >> inode->i_blkbits) + iblock);

	return 0;
}

/*
 * Read inline data to a pagecache
 */
static int raonfs_readpage_inline(struct inode *inode, struct page *page)
{
	struct raonfs_inode_info *ri = RAONFS_INODE(inode);
	struct buffer_head *bh;
	void *kaddr;

	bh = sb_getblk(inode->i_sb, ri->doffset >> inode->i_blkbits);
	if (unlikely(bh == NULL))
		return -ENOMEM;

	kaddr = kmap_atomic(page);
	memcpy(kaddr, &bh->b_data[ri->doffset & ((1 << inode->i_blkbits) - 1)], inode->i_size);
	flush_dcache_page(page);
	kunmap_atomic(kaddr);
	zero_user_segment(page, inode->i_size, PAGE_SIZE);
	SetPageUptodate(page);
	unlock_page(page);

	put_bh(bh);

	return 0;
}

/*
 * Read a pagecache
 */
static int raonfs_readpage(struct file *file, struct page *page)
{
	struct inode *inode = page->mapping->host;

	if (raonfs_test_inode_flag(inode, RAONFS_INODE_INLINE_DATA))
		return raonfs_readpage_inline(inode, page);

	return mpage_readpage(page, raonfs_get_block);
}

/*
 * Read pagecaches
 */
static int raonfs_readpages(struct file *file, struct address_space *mapping,
		struct list_head *pages, unsigned nr_pages)
{
	struct inode *inode = mapping->host;

	if (raonfs_test_inode_flag(inode, RAONFS_INODE_INLINE_DATA))
		return 0;

	return mpage_readpages(mapping, pages, nr_pages, raonfs_get_block);
}

const struct address_space_operations raonfs_address_space_operations = {
	.readpage = raonfs_readpage,
	.readpages = raonfs_readpages
};
