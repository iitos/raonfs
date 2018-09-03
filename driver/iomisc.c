#include <linux/fs.h>
#include <linux/buffer_head.h>
#include "raonfs.h"

/*
 * Read memory data on a block device
 */
size_t raonfs_block_read_memory(struct super_block *sb, unsigned long pos, void *buf, size_t len)
{
	struct buffer_head *bh;
	unsigned long offset;
	size_t segment;
	size_t remains;

	for (remains = len; remains > 0; buf += segment, remains -= segment, pos += segment) {
		offset = pos & (sb->s_blocksize - 1);
		segment = min_t(size_t, remains, sb->s_blocksize - offset);
		bh = sb_bread(sb, pos >> sb->s_blocksize_bits);
		if (bh == NULL)
			return -EIO;
		memcpy(buf, bh->b_data + offset, segment);
		brelse(bh);
	}

	return len;
}

/*
 * Read string data on a block device
 */
size_t raonfs_block_read_string(struct super_block *sb, unsigned long pos, char *buf, size_t len)
{
	struct buffer_head *bh;
	unsigned long offset;
	size_t segment;
	size_t remains;

	for (remains = len; remains > 0; buf += segment, remains -= segment, pos += segment) {
		offset = pos & (sb->s_blocksize - 1);
		segment = min_t(size_t, remains, sb->s_blocksize - offset);
		bh = sb_bread(sb, pos >> sb->s_blocksize_bits);
		if (bh == NULL)
			return -EIO;
		memcpy(buf, bh->b_data + offset, segment);
		brelse(bh);
	}

	buf[0] = '\0';

	return len;
}
