/*
 * Authors: Wolf Honore (whonore), Peter Finn ()
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

int main(int argc, char **argv) {
    printf ("state offset is %lx\n", __builtin_offsetof (struct prinfo, state));
    printf ("pid offset is %lx\n", __builtin_offsetof (struct prinfo, pid));
    printf ("parent_pid offset is %lx\n", __builtin_offsetof (struct prinfo, parent_pid));
    printf ("youngest_child_pid offset is %lx\n", __builtin_offsetof (struct prinfo, youngest_child_pid));
    printf ("younger_sibling_pid offset is %lx\n", __builtin_offsetof (struct prinfo, younger_sibling_pid));
    printf ("older_sibling_pid offset is %lx\n", __builtin_offsetof (struct prinfo, older_sibling_pid));
    printf ("start_time offset is %lx\n", __builtin_offsetof (struct prinfo, start_time));
    printf ("user_time offset is %lx\n", __builtin_offsetof (struct prinfo, user_time));
    printf ("sys_time offset is %lx\n", __builtin_offsetof (struct prinfo, sys_time));
    printf ("cutime offset is %lx\n", __builtin_offsetof (struct prinfo, cutime));
    printf ("cstime offset is %lx\n", __builtin_offsetof (struct prinfo, cstime));
    printf ("uid offset is %lx\n", __builtin_offsetof (struct prinfo, uid));
    printf ("comm offset is %lx\n", __builtin_offsetof (struct prinfo, comm));
    printf ("signal offset is %lx\n", __builtin_offsetof (struct prinfo, signal));
    printf ("num_open_fds offset is %lx\n", __builtin_offsetof (struct prinfo, num_open_fds));

    char* result = test1() ? "succeeded" : "failed";
    printf("Test 1 %s!\n", result);
    result = test2() ? "succeeded" : "failed";
    printf("Test 2 %s!\n", result);
    result = test3() ? "succeeded" : "failed";
    printf("Test 3 %s!\n", result);
    result = test4() ? "succeeded" : "failed";
    printf("Test 4 %s!\n", result);

    struct prinfo *info = (struct prinfo *) malloc(sizeof(struct prinfo));
    info->pid = getpid();
    
	if (syscall(181, info) < 0) {
	    perror("prinfo failed");
	    return 0;
	}
    
    printf("Run ps now.\n");
    printf("state is %ld\npid is %d\nparent_pid is %d\nyoungest_child_pid is %d\nyounger_sibling_pid is %d\nolder_sibling_pid is %d\nstart_time is %lu\nuser_time is %lu\nsys_time is %lu\ncutime is %lu\ncstime is %lu\nuid is %ld\ncomm is %s\nsignal is %lu\nnum_open_fds is %lu\n", info->state, info->pid, info->parent_pid, info->youngest_child_pid, info->younger_sibling_pid, info->older_sibling_pid, info->start_time, info->user_time, info->sys_time, info->cutime, info->cstime, info->uid, info->comm, info->signal, info->num_open_fds);
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
            
            //DELETE
            printf("Before our system call:\n");
            printf("grandparent's child = %d\n", grandparent_info->youngest_child_pid);
            printf("parent = %d\n", parent_info->pid);
            printf("grandchild's parent = %d\n\n", grandchild_info->parent_pid);
            
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
            
            //DELETE
            printf("grandparent's child = %d\n", grandparent_info->youngest_child_pid);
            printf("parent = %d\n", parent_info->pid);
            printf("grandchild's parent = %d\n\n", grandchild_info->parent_pid);
            
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
            return 0;
    }
    else {
        perror("fork failed");
        return 0;
    }
    
    return 0;
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
    
    printf("in test2:\nyounger = %d\nyounger of older = %d\n", younger->pid, older->younger_sibling_pid);
    printf("in test2:\nolder = %d\nolder of younger = %d\n", younger->pid, older->younger_sibling_pid);
    
    /* Older's younger sibling should be younger and vice-versa */
    return ((older->younger_sibling_pid == younger->pid) 
            && (younger->older_sibling_pid == older->pid));
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

