/*
 * Authors: Wolf Honore (whonore), Peter Finn (pfinn2)
 * Assignment: MP 2 CSC 456 Fall 2015
 *
 * Description:
 *   This program tests the prinfo system call by checking values in the prinfo
 *   struct against expected values.
 */

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <linux/unistd.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "prinfo.h"

int test1();
int test2();
int test3();
int test4();
int test5();
void print_info(struct prinfo *info);

/*
 * Function: main()
 *
 * Description:
 *   This function is the entry point for the test program. It calls each of
 *   the test functions and prints the results. It then calls the prinfo
 *   system call and prints all of the fields.
 *
 * Inputs:
 *   argc - The number of command line arguments.
 *   argv - The list of command line arguments.
 *
 * Outputs: None
 *
 * Return value:
 *   0 - Normal exit.
 *   Any other number - Error.
 */
int main(int argc, char **argv) {
	char* result = test1() ? "succeeded" : "failed";
	printf("Test 1 %s!\n", result);
	result = test2() ? "succeeded" : "failed";
	printf("Test 2 %s!\n", result);
	result = test3() ? "succeeded" : "failed";
	printf("Test 3 %s!\n", result);
	result = test4() ? "succeeded" : "failed";
	printf("Test 4 %s!\n", result);
	result = test5() ? "succeeded" : "failed";
	printf("Test 5 %s!\n", result);

	struct prinfo *info = (struct prinfo *) malloc(sizeof(struct prinfo));
	info->pid = getpid();

	if (syscall(181, info) < 0) {
		perror("prinfo failed");
		return 0;
	}

	print_info(info);
	printf("Run ps now.\n");
	sleep(100);

	return 0;
}

/*
 * Function: test1()
 *
 * Description:
 *   This function tests for accuracy of PIDs between parents and children.
 *
 * Inputs: None
 *
 * Outputs: None
 *
 * Return value:
 *   0 - This test failed, and so prinfo recognition of parent/child
 *       relationships is buggy.
 *   1 - This test succeeded.
 */
int test1() {
	pid_t fork_result1, fork_result2;
	pid_t grandparent_pid = getpid();
	pid_t parent_pid = -1;
	pid_t grandchild_pid = -1;
	int waitstatus1, waitstatus2;

	if ((fork_result1 = fork()) == 0) {
		if ((fork_result2 = fork()) == 0) {
			/* Within grandchild */
			sleep(10);
			exit(0);
		}
		else if (fork_result2 > 0) {
			/* Within parent */
			struct prinfo *grandparent_info, *parent_info, *grandchild_info;
			parent_pid = getpid();
			grandchild_pid = fork_result2;

			grandparent_info = (struct prinfo *) malloc(sizeof(struct prinfo));
			parent_info = (struct prinfo *) malloc(sizeof(struct prinfo));
			grandchild_info = (struct prinfo *) malloc(sizeof(struct prinfo));

			grandparent_info->pid = grandparent_pid;
			parent_info->pid = parent_pid;
			grandchild_info->pid = grandchild_pid;

			if (syscall(181, grandparent_info) < 0) {
				perror("prinfo failed");
				return 0;
			}
			if (syscall(181, parent_info) < 0) {
				perror("prinfo failed");
				return 0;
			}
			if (syscall(181, grandchild_info) < 0) {
				perror("prinfo failed");
				return 0;
			}

			/* Grandparent's youngest child and grandchild's parent should be parent */
			int ret1 = (grandparent_info->youngest_child_pid == parent_info->pid
						&& parent_info->pid == grandchild_info->parent_pid);

			/* Parent's parent should be grandparent */
			int ret2 = (parent_info->parent_pid == grandparent_info->pid);

			/* Parent's youngest child should be grandchild */
			int ret3 = (grandchild_info->pid == parent_info->youngest_child_pid);

			if (waitpid(fork_result2, &waitstatus1, 0) == -1)
				return 0;
			return (ret1 && ret2 && ret3);
		}
		else {
			perror("fork failed");
			return 0;
		}

		exit(0);
	}
	else if (fork_result1 > 0) {
		/* Within grandparent */
		if (waitpid(fork_result1, &waitstatus2, 0) == -1)
			exit(0);
	}
	else {
		perror("fork failed");
		return 0;
	}

	exit(0);
}

/*
 * Function: test2()
 *
 * Description:
 *   This function tests for correctness of sibling relationships between
 *   processes.
 *
 * Inputs: None
 *
 * Outputs: None
 *
 * Return value:
 *   0 - The test failed, and prinfo recognition of sibling relationships is
 *       buggy.
 *   1 - The test succeeded.
 */
int test2() {
	struct prinfo *older, *younger;
	pid_t fork_result1, fork_result2;
	int waitstatus;

	older = (struct prinfo *) malloc(sizeof(struct prinfo));
	younger = (struct prinfo *) malloc(sizeof(struct prinfo));

	/* Spawn two child processes */
	if ((fork_result1 = fork()) > 0) {
		if ((fork_result2 = fork()) > 0) {
			older->pid = fork_result1;
			younger->pid = fork_result2;

			if (syscall(181, older) < 0) {
				perror("prinfo failed");
				return 0;
			}
			if (syscall(181, younger) < 0) {
			   perror("prinfo failed");
			   return 0;
			}

			if (waitpid(fork_result1, &waitstatus, 0) == -1) return 0;
			if (waitpid(fork_result2, &waitstatus, 0) == -1) return 0;
		}
		else if (fork_result2 == 0) {
			sleep(10);
			exit(0);
		}
		else {
			perror("fork failed!");
			return 0;
		}
	}
	else if (fork_result1 == 0) {
		sleep(10);
		exit(0);
	}
	else {
		perror("fork failed");
		return 0;
	}

	/* Older's younger sibling should be younger and vice-versa */
	int ret1 = ((older->younger_sibling_pid == younger->pid)
				&& (younger->older_sibling_pid == older->pid));

	/* Older should be older than younger */
	int ret2 = (older->start_time < younger->start_time);

	return (ret1 && ret2);
}

/*
 * Function: test3()
 *
 * Description:
 *   This function tests to check the accuracy of prinfo's open FD count.
 *
 * Inputs: None
 *
 * Outputs: None
 *
 * Return value:
 *   0 - The test failed, and prinfo's num_open_fds field is buggy.
 *   1 - The test succeeded.
 */
int test3() {
	int fd1 = open("fd1.txt", O_RDWR | O_CREAT);
	int fd2 = open("fd2.txt", O_RDWR | O_CREAT);
	if (fd1 == -1 || fd2 == -1) {
		fprintf(stderr, "Error opening files\n");
		return 0;
	}

	struct prinfo *info;
	info = (struct prinfo *) malloc(sizeof(struct prinfo));

	info->pid = getpid();

	if (syscall(181, info) < 0) {
		perror("prinfo failed");
		return 0;
	}

	int ret1 = (info->num_open_fds == 5);
	if (close(fd2) < 0) {
		perror("close failed");
		return 0;
	}

	if (syscall(181, info) < 0) {
		perror("prinfo failed");
		return 0;
	}

	int ret2 = (info->num_open_fds == 4);
	if (close(fd1) < 0) {
		perror("close failed");
		return 0;
	}

	if (syscall(181, info) < 0) {
		perror("prinfo failed");
		return 0;
	}

	int ret3 = (info->num_open_fds == 3);

	return (ret1 && ret2 && ret3);
}

/*
 * Function: test4()
 *
 * Description:
 *   This function tests that pending signals are correctly stored in the
 *   prinfo struct.
 *
 * Inputs: None
 *
 * Outputs: None
 *
 * Return value:
 *   0 - This test failed, and so the pending signal set is not correctly
 *       stored.
 *   1 - This test succeeded.
 */
int test4() {
	struct prinfo *info;

	info = (struct prinfo *) malloc(sizeof(struct prinfo));

	info->pid = getpid();

	/* Blocks SIGUSR2 and then sends SIGUSR2 to this process */
	sigset_t block;
	sigemptyset(&block);
	sigaddset(&block, SIGUSR2);
	sigprocmask(SIG_BLOCK, &block, NULL);
	if (kill(info->pid, SIGUSR2) < 0) {
		perror("kill failed");
		return 0;
	}

	if (syscall(181, info) < 0) {
		perror("prinfo failed");
		return 0;
	}

	/* Print out list of signals */
	int i;
	int max = 8 * sizeof(unsigned long);

	printf("The list of pending signals:\n");
	for (i = max - 1; i >= 0; i--) {
		printf("%d", (int) ((info->signal >> i) & 0x01));
	}
	printf("\n");

	return (info->signal = 2 << (SIGUSR2 - 1));
}

/*
 * Function: test5()
 *
 * Description:
 *   This function tests that the prinfo system call correctly handles invalid
 *   inputs.
 *
 * Inputs: None
 *
 * Outputs: None
 *
 * Return value:
 *   0 - The test failed, and prinfo does not return an error code for invalid
 *       input.
 *   1 - The test succeeded.
 */
int test5() {
	/* Test passing NULL struct */
	int ret1 = (syscall(181, NULL) < 0);

	/* Test unset PID */
	struct prinfo *info;
	info = (struct prinfo *) malloc(sizeof(struct prinfo));
	int ret2 = (syscall(181, info) < 0);

	/* Test invalid PID */
	info->pid = -1;
	int ret3 = (syscall(181, info) < 0);

	return (ret1 && ret2 && ret3);
}

void print_info(struct prinfo *info) {
	printf("\nProcess information:\n");

	char *state_text;
	switch (info->state) {
		case 0:
			state_text = "Running";
			break;
		case 1:
			state_text = "Interruptible";
			break;
		case 2:
			state_text = "Uninterruptible";
			break;
		case 4:
			state_text = "Stopped";
			break;
		case 8:
			state_text = "Traced";
			break;
		case 64:
			state_text = "Dead";
			break;
		case 128:
			state_text = "Wakekill";
			break;
		case 256:
			state_text = "Waking";
			break;
		case 512:
			state_text = "Parked";
			break;
		default:
			state_text = "Unknown";
	}
	printf("State: %ld (%s)\n", info->state, state_text);

	printf("PID: %d\nParent PID: %d\n", info->pid, info->parent_pid);
	printf("Youngest child PID: %d\n", info->youngest_child_pid);
	printf("Younger sibling PID: %d\nOlder sibling PID: %d\n",
		   info->younger_sibling_pid, info->older_sibling_pid);

	printf("Start time (nsec): %lu\nUser time (nsec): %lu\nSystem time (nsec): %lu\n",
		   info->start_time, info->user_time, info->sys_time);
	printf("Total child user time (nsec): %lu\nTotal child system time (nsec): %lu\n",
		   info->cutime, info->cstime);

	printf("User ID: %ld\nProgram name: %s\n", info->uid, info->comm);

	printf("Pending signals list: %lu\nOpen file descriptors: %lu\n",
		   info->signal, info->num_open_fds);
}

