#include <linux/types.h>
#include <linux/string.h>
#include <linux/printk.h>
#include <linux/sched.h>

void __raonfs_message(const char *function, unsigned int line, const char *tag, const char *fmt, ...)
{
	struct va_format vaf;
	va_list args;

	va_start(args, fmt);

	vaf.fmt = fmt;
	vaf.va = &args;

	printk(KERN_NOTICE "RAONFS: %s:%d: %s: %pV\n", function, line, tag, &vaf);

	va_end(args);
}

void __raonfs_error(const char *function, unsigned int line, const char *tag, const char *fmt, ...)
{
	struct va_format vaf;
	va_list args;

	va_start(args, fmt);

	vaf.fmt = fmt;
	vaf.va = &args;

	printk(KERN_CRIT "RAONFS error: %s:%d: comm %s: %s: %pV\n", function, line, current->comm, tag, &vaf);

	va_end(args);
}

void __raonfs_warning(const char *function, unsigned int line, const char *tag, const char *fmt, ...)
{
	struct va_format vaf;
	va_list args;

	va_start(args, fmt);

	vaf.fmt = fmt;
	vaf.va = &args;

	printk(KERN_WARNING "RAONFS warning: %s:%d: comm %s: %s: %pV\n", function, line, current->comm, tag, &vaf);

	va_end(args);
}
