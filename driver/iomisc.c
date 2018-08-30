#include <linux/fs.h>
#include <linux/buffer_head.h>
#include "raonfs.h"

/*
 * Read data on a block device
 */
size_t raonfs_block_read(struct super_block *sb, unsigned long pos, void *buf, size_t len)
{
	struct raonfs_sb_info *sbi = RAONFS_SB(sb);
	struct buffer_head *bh;
	unsigned long offset;
	size_t segment;
	size_t remains;

	if (pos >= sbi->fssize)
		return -EIO;
	if (len > sbi->fssize - pos)
		len = sbi->fssize - pos;

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
 * Read string on a block device
 */
size_t raonfs_block_strcpy(struct super_block *sb, unsigned long pos, char *buf, size_t len)
{
	struct raonfs_sb_info *sbi = RAONFS_SB(sb);
	struct buffer_head *bh;
	unsigned long offset;
	size_t segment;
	size_t remains;

	if (pos >= sbi->fssize)
		return -EIO;
	if (len > sbi->fssize - pos)
		len = sbi->fssize - pos;

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

/*
 * Get the length of a string on a block device
 */
ssize_t raonfs_block_strlen(struct super_block *sb, unsigned long pos, size_t limit)
{
	struct raonfs_sb_info *sbi = RAONFS_SB(sb);
	struct buffer_head *bh;
	unsigned long offset;
	size_t segment;
	ssize_t len = 0;
	u_char *buf, *p;

	if (pos >= sbi->fssize)
		return -EIO;
	if (limit > sbi->fssize - pos)
		limit = sbi->fssize - pos;

	for (; limit > 0; limit -= segment, pos += segment, len += segment) {
		offset = pos & (sb->s_blocksize - 1);
		segment = min_t(size_t, limit, sb->s_blocksize - offset);
		bh = sb_bread(sb, pos >> sb->s_blocksize_bits);
		if (bh == NULL)
			return -EIO;
		buf = bh->b_data + offset;
		p = memchr(buf, 0, segment);
		brelse(bh);
		if (p != NULL)
			return len + (p - buf);
	}

	return len;
}

/*
 * Compare two strings on a block device
 */
int raonfs_block_strcmp(struct super_block *sb, unsigned long pos, const char *str, size_t size)
{
	struct raonfs_sb_info *sbi = RAONFS_SB(sb);
	struct buffer_head *bh;
	unsigned long offset;
	size_t segment;
	bool matched;
	bool terminated = false;

	if (pos >= sbi->fssize)
		return -EIO;
	if (size + 1 > sbi->fssize - pos)
		return -EIO;

	for (; size > 0; size -= segment, pos += segment, str += segment) {
		offset = pos & (sb->s_blocksize - 1);
		segment = min_t(size_t, size, sb->s_blocksize - offset);
		bh = sb_bread(sb, pos >> sb->s_blocksize_bits);
		if (bh == NULL)
			return -EIO;
		matched = memcmp(bh->b_data + offset, str, segment) == 0;
		if (matched && size == 0 && offset + segment < sb->s_blocksize) {
			if (!bh->b_data[offset + segment])
				terminated = true;
			else
				matched = false;
		}
		brelse(bh);
		if (!matched)
			return 1;
	}

	if (!terminated) {
		bh = sb_bread(sb, pos >> sb->s_blocksize_bits);
		if (bh == NULL)
			return -EIO;
		matched = !bh->b_data[0];
		brelse(bh);
		if (!matched)
			return 1;
	}

	return 0;
}
