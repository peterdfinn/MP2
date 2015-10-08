#include <linux/kernel.h>
#include <linux/prinfo.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/syscalls.h>
#include <linux/types.h>

SYSCALL_DEFINE1(prinfo, struct prinfo *, info) 
{
	struct prinfo *kinfo;
	struct task_struct *task;
	pid_t pid;

	kinfo = (struct prinfo *) kmalloc(sizeof(struct prinfo), GFP_KERNEL);	
	copy_from_user(kinfo, info, sizeof(struct prinfo));
	
	pid = kinfo->pid;

	task = pid_task(find_vpid(pid), PIDTYPE_PID);

	/* Time stats */
	kinfo->user_time = task->utime;
	kinfo->sys_time = task->stime;
	kinfo->cutime = 0;
	kinfo->cstime = 0;

	/* User ID */
	kinfo->uid = 0;

	/* Program name */
	strncpy(kinfo->comm, task->comm, 15);
	kinfo->comm[15] = '\0';
	
	/* Signals */
	kinfo->signal = 0;

	/* Open file descriptors */
	kinfo->num_open_fds = 0;

	copy_to_user(info, kinfo, sizeof(struct prinfo));

	return 0;
};
