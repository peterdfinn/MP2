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
static void sum_children_time(struct task_struct *task, struct prinfo *info);
static void get_pending(struct task_struct *task, unsigned long *out_pending);
static unsigned long sigset_to_long(sigset_t pending_set);

/*
 *
 */
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
	sum_children_time(task, kinfo);

	/* User ID */
	kinfo->uid = task->real_cred->uid.val;

	/* Program name */
	strncpy(kinfo->comm, task->comm, 15);
	kinfo->comm[15] = '\0';
	
	/* Signals */ 
	get_pending(task, &kinfo->signal);
	
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

/*
 *
 */
static void sum_children_time(struct task_struct *task, struct prinfo *info)
{
	struct task_struct *child;
	struct list_head *child_list;

	info->cutime = 0;
	info->cstime = 0;

	list_for_each(child_list, &task->children) {
		child = list_entry(child_list, struct task_struct, sibling);
		info->cutime += child->utime;
		info->cstime += child->stime;
	}
}

/*
 * Mostly copied from kernel/signal.c do_sigpending()
 */
static void get_pending(struct task_struct *task, unsigned long *out_pending)
{
	sigset_t pending_set;

	spin_lock_irq(&task->sighand->siglock);
	sigorsets(&pending_set, &task->pending.signal, &task->signal->shared_pending.signal);
	spin_unlock_irq(&task->sighand->siglock);
	
	sigandsets(&pending_set, &task->blocked, &pending_set);

	*out_pending = sigset_to_long(pending_set);
}

/*
 *
 */
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
