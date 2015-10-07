#include <linux/kernel.h>
#include <linux/prinfo.h>
#include <linux/syscalls.h>

SYSCALL_DEFINE1(prinfo, struct prinfo *, info) 
{
	printk("Syscall hi.\n");
	return 0;
};
