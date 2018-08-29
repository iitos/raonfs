#ifndef __RAONFS_DBMISC_H__
#define __RAONFS_DBMISC_H__

#define raonfs_notice(fmt, ...)	\
	__raonfs_notice(__func__, __LINE__, fmt, ##__VA_ARGS__)
#define raonfs_error(fmt, ...)	\
	__raonfs_error(__func__, __LINE__, fmt, ##__VA_ARGS__)
#define raonfs_warning(fmt, ...)	\
	__raonfs_warning(__func__, __LINE__, fmt, ##__VA_ARGS__)

extern void __raonfs_notice(const char *function, unsigned int line, const char *fmt, ...);
extern void __raonfs_error(const char *function, unsigned int line, const char *fmt, ...);
extern void __raonfs_warning(const char *function, unsigned int line, const char *fmt, ...);

#endif
