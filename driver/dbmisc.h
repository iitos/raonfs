#ifndef __RAONFS_DBMISC_H__
#define __RAONFS_DBMISC_H__

#define raonfs_message(tag, fmt, ...)	\
	__raonfs_message(__func__, __LINE__, tag, fmt, ##__VA_ARGS__)
#define raonfs_error(tag, fmt, ...)	\
	__raonfs_error(__func__, __LINE__, tag, fmt, ##__VA_ARGS__)
#define raonfs_warning(tag, fmt, ...)	\
	__raonfs_warning(__func__, __LINE__, tag, fmt, ##__VA_ARGS__)

extern void __raonfs_message(const char *function, unsigned int line, const char *tag, const char *fmt, ...);
extern void __raonfs_error(const char *function, unsigned int line, const char *tag, const char *fmt, ...);
extern void __raonfs_warning(const char *function, unsigned int line, const char *tag, const char *fmt, ...);

#endif
