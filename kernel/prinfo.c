/*
 * Authors: Wolf Honore (whonore), Peter Finn (pfinn2)
 * Assignment: MP 2 CSC 456 Fall 2015
 *
 * Description:
 *   This program implements the prinfo system call. It contains the defintion
 *   for the system call itself as well as several helper functions.
 */

#include <asm/cputime.h>
#include <asm/signal.h>
#include <linux/fdtable.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/prinfo.h>
#include <linux/syscalls.h>
#include <linux/signal.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/types.h>

static int count_open_files(struct fdtable *fdt);
static void sum_children_time(struct task_struct *task, struct prinfo *info);
static unsigned long get_pending(struct task_struct *task);
static unsigned long sigset_to_long(sigset_t set);

/*
 * Function: prinfo()
 *
 * Description: 
 *   This function is a system call that returns information about a given
 *   process. This information includes: 
 *     - The state of the process 
 *     - The PIDs of the parent, youngest child, oldest among the younger 
 *       siblings, and youngest among the older siblings processes
 *     - The process start time
 *     - The CPU time spent in user and system mode as well as the total child
 *       user and system times
 *     - The user id of the process owner
 *     - The name of the program
 *     - The set of pending signals
 *     - The number of open files
 *
 * Inputs:
 *   info - A pointer to a struct provided by the user with the pid field
 *          already set. prinfo uses the pid field to compute the rest of the 
 *          fields and then returns info back to the user.
 *
 * Outputs:
 *   info - The pointer to the struct that the user provided is filled with 
 *          information about the requested process and returned to the user.
 *
 * Return value:
 *   0 - Success
 *   -EINVAL - The info struct is NULL or the requested PID is invalid.
 *   -EFAULT - The memory pointed to by info is invalid and cannot be copied
 *             from.
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

	/* State */
	kinfo->state = task->state;

	/* PIDs of parent, youngest child, and older and younger siblings */
	kinfo->parent_pid = task->parent->pid;
	 
	kinfo->youngest_child_pid = list_first_entry(&task->children, struct task_struct, sibling)->pid;

	/* Set PID to -1 if child does not exist */
	if (kinfo->youngest_child_pid == kinfo->pid) 
	    kinfo->youngest_child_pid = -1;

	kinfo->older_sibling_pid = list_prev_entry(task, sibling)->pid;
	kinfo->younger_sibling_pid = list_next_entry(task, sibling)->pid;
	
	/* Set PID to -1 if sibling does not exist */
	if (kinfo->older_sibling_pid == 0)
	    kinfo->older_sibling_pid = -1;
	if (kinfo->younger_sibling_pid == 0)
	    kinfo->younger_sibling_pid = -1;

	/* Time stats */
	kinfo->start_time = (unsigned long) task->start_time;
	kinfo->user_time = cputime_to_nsecs(task->utime);
	kinfo->sys_time = cputime_to_nsecs(task->stime);
	sum_children_time(task, kinfo);

	/* User ID */
	kinfo->uid = task->real_cred->uid.val;

	/* Program name */
	strncpy(kinfo->comm, task->comm, 15);
	kinfo->comm[15] = '\0';
	
	/* Signals */ 
	kinfo->signal = get_pending(task);
	
	/* Open file descriptors */ 
	files = get_files_struct(task);
	files_table = files_fdtable(files);
	kinfo->num_open_fds = count_open_files(files_table);

	/* Copy struct back to user space */
	if (copy_to_user(info, kinfo, sizeof(struct prinfo)))
		return -EFAULT;
	
	return 0;
}

/*
 * Function: count_open_files()
 *
 * Description:
 *   This function uses the open_fds field of the fdtable struct to count a
 *   process' number of open file descriptors.
 *
 * Inputs:
 *   fdt - The struct containing information about a processes' open files.
 *
 * Outputs: None
 *
 * Return value:
 *   The current number of open files (ranges from 0 to the maximum number of
 *   open files).
 */
static int count_open_files(struct fdtable *fdt) 
{ 
    int max = fdt->max_fds;
	long open_fs = *(fdt->open_fds);
	int count = 0;
    int i; 

	/* The i-th bit in open_fs represents whether file i is open */ 
    for (i = 0; i < max; i++)  
        count += ((open_fs >> i) & 0x01);
    
	return count; 
}

/*
 * Function: sum_children_time()
 *
 * Description:
 *   This function sums the total user and system time of each of a process'
 *   children.
 *
 * Inputs:
 *   task - The struct containing information about a process including the
 *          list of child processes.
 *   
 * Outputs:
 *   info - The struct in which the total user and system time of a process'
 *          children is stored.
 *
 * Return value: None
 */
static void sum_children_time(struct task_struct *task, struct prinfo *info)
{
	struct task_struct *child;
	struct list_head *child_list;

	info->cutime = 0;
	info->cstime = 0;

	/* Loop through the list of children */
	list_for_each(child_list, &task->children) {
		child = list_entry(child_list, struct task_struct, sibling);
		info->cutime += child->utime;
		info->cstime += child->stime;
		
		printk("child of %d %d %d %lu\n",task->pid, child->pid, child->parent->pid, (unsigned long) child->start_time);
	}
	
	info->cutime = cputime_to_nsecs(info->cutime);
	info->cstime = cputime_to_nsecs(info->cstime);
}

/*
 * Function: get_pending()
 *
 * Description:
 *   This function checks a process' list of pending signals and returns an
 *   unsigned long number representing this list. Much of the code is copied 
 *   from the do_sigpending() function in kernel/signal.c. 
 *   
 * Inputs:
 *   task - The struct containing information about a process including the
 *          list of pending signals.
 *
 * Ouputs: None
 *
 * Return value:
 *   An unsigned long number where the i-th bit represents whether signal i+1
 *   is pending.
 */
static unsigned long get_pending(struct task_struct *task) 
{
	sigset_t pending_set;

	/* Combine the pending and shared_pending lists */
	spin_lock_irq(&task->sighand->siglock);
	sigorsets(&pending_set, &task->pending.signal, &task->signal->shared_pending.signal);
	spin_unlock_irq(&task->sighand->siglock);
	
	/* Keep only the signals that are currently blocked */
	sigandsets(&pending_set, &task->blocked, &pending_set);

	return sigset_to_long(pending_set);
}

/*
 * Function: sigset_to_long()
 *
 * Description:
 *   This function converts a set of signals stored in a sigset_t type variable
 *   to an unsigned long number representing the same set. 
 *
 * Inputs:
 *   set - This is some set of signals.
 *
 * Outputs: None
 *
 * Return value:
 *   An unsigned long number where the i-th bit represents whether signal i+1
 *   is in the set of signals.
 */
static unsigned long sigset_to_long(sigset_t set) 
{
	unsigned long signals;	
	int sig;

	/* If signal i is in the set, set the i-th bit of signals to 1 */	
	signals = 0;
	for (sig = 1; sig < _NSIG; sig++) 
		if (sigismember(&set, sig)) 
			signals |= sigmask(sig);
		
	return signals;
} 

