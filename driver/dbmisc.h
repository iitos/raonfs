#ifndef __RAONFS_DBMISC_H__
#define __RAONFS_DBMISC_H__

#define raonfs_message(fmt, ...)	\
	__raonfs_message(__func__, __LINE__, fmt, ##__VA_ARGS__)
#define raonfs_error(fmt, ...)	\
	__raonfs_error(__func__, __LINE__, fmt, ##__VA_ARGS__)
#define raonfs_warning(fmt, ...)	\
	__raonfs_warning(__func__, __LINE__, fmt, ##__VA_ARGS__)

extern void __raonfs_message(const char *function, unsigned int line, const char *fmt, ...);
extern void __raonfs_error(const char *function, unsigned int line, const char *fmt, ...);
extern void __raonfs_warning(const char *function, unsigned int line, const char *fmt, ...);

#endif
