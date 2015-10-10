#include <asm/signal.h>
#include <linux/fdtable.h>
#include <linux/kernel.h>
#include <linux/prinfo.h>
#include <linux/signal.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/syscalls.h>
#include <linux/types.h>

static int count_open_files(struct fdtable *fdt);
static unsigned long sigset_to_long(sigset_t pending_set);

SYSCALL_DEFINE1(prinfo, struct prinfo *, info) 
{
	struct prinfo *kinfo;
	struct task_struct *task;
	struct files_struct *files;
	struct fdtable *files_table;	
	pid_t pid;

	if (info == NULL)
		return -EINVAL;

	/* Copy struct from user space */
	kinfo = (struct prinfo *) kmalloc(sizeof(struct prinfo), GFP_KERNEL);	
	if (copy_from_user(kinfo, info, sizeof(struct prinfo)))
		return -EFAULT;	

	pid = kinfo->pid;

	if ((task = pid_task(find_vpid(pid), PIDTYPE_PID)) == NULL)
		return -EINVAL;

	/* Time stats */
	kinfo->user_time = task->utime;
	kinfo->sys_time = task->stime;
	kinfo->cutime = 0;
	kinfo->cstime = 0;

	/* User ID */
	kinfo->uid = task->real_cred->uid.val;

	/* Program name */
	strncpy(kinfo->comm, task->comm, 15);
	kinfo->comm[15] = '\0';
	
	/* Signals */ 
	kinfo->signal = sigset_to_long(task->pending.signal);
	
	sigfillset(&task->pending.signal);
	kinfo->signal = sigset_to_long(task->pending.signal);

	/* Open file descriptors */ 
	files = get_files_struct(task);
	files_table = files_fdtable(files);

	kinfo->num_open_fds = count_open_files(files_table);

	/* Copy struct back to user space */
	if (copy_to_user(info, kinfo, sizeof(struct prinfo)))
		return -EFAULT;

	return 0;
};

/*
 * 
 */
static int count_open_files(struct fdtable *fdt) 
{ 
        int max = fdt->max_fds;
	long open_fs = *(fdt->open_fds);
	int count = 0;
        int i; 
 
        for (i = 0; i < max; i++) { 
                count += ((open_fs >> i) & 0x01);
        }

	return count; 
}


static unsigned long sigset_to_long(sigset_t pending_set) 
{
	unsigned long pending;	
	int sig;
	
	pending = 0;
	for (sig = 1; sig < _NSIG; sig++) {
		if (sigismember(&pending_set, sig)) {
			pending |= sigmask(sig);
		}
	}
	
	return pending;
} 
