#include <linux/fs.h>
#include <linux/capability.h>
#include "xattr.h"

static bool raonfs_xattr_security_list(struct dentry *dentry)
{
	return capable(CAP_SYS_ADMIN);
}

static int raonfs_xattr_security_get(const struct xattr_handler *handler,
		struct dentry *unused, struct inode *inode,
		const char *name, void *buffer, size_t size)
{
	return -ENODATA;
}

static int raonfs_xattr_security_set(const struct xattr_handler *handler,
		struct dentry *unused, struct inode *inode,
		const char *name, const void *value,
		size_t size, int flags)
{
	return -ENODATA;
}

static const struct xattr_handler raonfs_xattr_security_handler = {
	.prefix = XATTR_SECURITY_PREFIX,
	.list = raonfs_xattr_security_list,
	.get = raonfs_xattr_security_get,
	.set = raonfs_xattr_security_set
};

static bool raonfs_xattr_trusted_list(struct dentry *dentry)
{
	return capable(CAP_SYS_ADMIN);
}

static int raonfs_xattr_trusted_get(const struct xattr_handler *handler,
		struct dentry *unused, struct inode *inode,
		const char *name, void *buffer, size_t size)
{
	return -ENODATA;
}

static int raonfs_xattr_trusted_set(const struct xattr_handler *handler,
		struct dentry *unused, struct inode *inode,
		const char *name, const void *value,
		size_t size, int flags)
{
	return -ENODATA;
}

static const struct xattr_handler raonfs_xattr_trusted_handler = {
	.prefix = XATTR_TRUSTED_PREFIX,
	.list = raonfs_xattr_trusted_list,
	.get = raonfs_xattr_trusted_get,
	.set = raonfs_xattr_trusted_set
};

static bool raonfs_xattr_user_list(struct dentry *dentry)
{
	return capable(CAP_SYS_ADMIN);
}

static int raonfs_xattr_user_get(const struct xattr_handler *handler,
		struct dentry *unused, struct inode *inode,
		const char *name, void *buffer, size_t size)
{
	return -ENODATA;
}

static int raonfs_xattr_user_set(const struct xattr_handler *handler,
		struct dentry *unused, struct inode *inode,
		const char *name, const void *value,
		size_t size, int flags)
{
	return -ENODATA;
}

static const struct xattr_handler raonfs_xattr_user_handler = {
	.prefix = XATTR_USER_PREFIX,
	.list = raonfs_xattr_user_list,
	.get = raonfs_xattr_user_get,
	.set = raonfs_xattr_user_set
};

const struct xattr_handler *raonfs_xattr_handlers[] = {
	&raonfs_xattr_user_handler,
	&raonfs_xattr_trusted_handler,
	&raonfs_xattr_security_handler,
	NULL
};
