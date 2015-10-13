#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <linux/unistd.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "prinfo.h"

void print_all(struct prinfo *info);
void print_bin(unsigned long l);

/*
 * Function: main()
 *
 * Description:
 *   Entry point for this program.
 *
 * Inputs:
 *   argc - The number of argument with which this program was executed.
 *   argv - Array of pointers to strings containing the command-line arguments. 
 *
 * Return value:
 *   0 - This program terminated normally.
 */
int
main (int argc, char ** argv) {
	struct prinfo *info = (struct prinfo *) malloc(sizeof(struct prinfo));
	int fd;

	if (argc == 1) {
		info->pid = getpid();
		printf("Pid: %d\n", info->pid);

		printf("\nRun 1: No open files, no children, block SIGUSR2, send SIGUSR2\n");

		sigset_t block;
		sigemptyset(&block);
		sigaddset(&block, SIGUSR2);
		sigaddset(&block, SIGHUP);		
		sigprocmask(SIG_BLOCK, &block, NULL);
		kill(info->pid, SIGUSR2);
		kill(info->pid, SIGHUP);

		if (syscall(181, info) < 0)
			perror("1");
		
		print_all(info);

		printf("\nRun 2: One open file, two children, block SIGUSR1 and SIGUSR2, send SIGUSR1 and SIGUSR2\n");

		if ((fd = open("afile", 0)) == -1) {
			printf("Didn't open\n");
		};

		pid_t c1, c2;
		if ((c1 = fork()) == 0) {
			sleep(15);
			exit(0);
		}
		if ((c2 = fork()) == 0) {
			sleep(30);
			exit(0);
		}

		sigaddset(&block, SIGUSR1);
		sigprocmask(SIG_BLOCK, &block, NULL);

		kill(info->pid, SIGUSR1);
		kill(info->pid, SIGUSR2);

		if (syscall(181, info) < 0) 
			perror("2");

		print_all(info);

		printf("\nRun 3: No open files, one child\n");
	
		if (close(fd) == -1)
			printf("Didn't close\n");

		wait(NULL);

		if (syscall(181, info) < 0)
			perror("3");

		print_all(info);
	}
	else {
		info->pid = atoi(argv[1]);
		printf("Pid: %d\n", info->pid);

		if (syscall(181, info) < 0)
			perror("1");
		
		print_all(info);
	}
	/* Exit the program */
	return 0;
}

void print_all(struct prinfo *info) {
	printf("sys_time: %lu\ncutime: %lu\ncstime: %lu\nuid: %ld\ncomm: %s\nsignal: %lu\nnum_open_fds: %lu\n", info->sys_time, info->cutime, info->cstime, info->uid, info->comm, info->signal, info->num_open_fds);
	printf("signals: ");
	print_bin(info->signal);
}

void print_bin(unsigned long l) {
	int i;
	int max = 8 * sizeof(l);

	for (i = max - 1; i >= 0; i--) {
		printf("%d", (int) ((l >> i) & 0x01));
	}

	printf("\n");
}
