#ifndef __RAONFS_IOMISC_H__
#define __RAONFS_IOMISC_H__

extern size_t raonfs_block_read(struct super_block *sb, unsigned long pos, void *buf, size_t len);
extern ssize_t raonfs_block_strlen(struct super_block *sb, unsigned long pos, size_t limit);
extern int raonfs_block_strcmp(struct super_block *sb, unsigned long pos, const char *str, size_t size);

#endif
