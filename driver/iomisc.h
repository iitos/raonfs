#ifndef __RAONFS_IOMISC_H__
#define __RAONFS_IOMISC_H__

extern size_t raonfs_block_read_memory(struct super_block *sb, unsigned long pos, void *buf, size_t len);
extern size_t raonfs_block_read_string(struct super_block *sb, unsigned long pos, char *buf, size_t len);

#endif
