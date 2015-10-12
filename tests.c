#include <sys/types.h>
#include <sys/wait.h>
#include <linux/unistd.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

struct prinfo {
  long state; // current state of process
  pid_t pid; // process id (input)
  pid_t parent_pid; // process id of parent
  pid_t youngest_child_pid; // process id of youngest child
  pid_t younger_sibling_pid; // pid of the oldest among younger siblings
  pid_t older_sibling_pid; // pid of the youngest among older siblings
  struct timespec start_time; // process start time
  unsigned long user_time; // CPU time spent in user mode
  unsigned long sys_time; // CPU time spent in system mode
  unsigned long cutime; // total user time of children
  unsigned long cstime; // total system time of children
  long uid; // user id of process owner
  char comm[16]; // name of program executed
  unsigned long signal; // The set of pending signals
  unsigned long num_open_fds; // Number of open file descriptors
};

int test1();
int test2();
int test3();

int main() {
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
  
  struct prinfo info;
  info.pid = getpid();
  syscall(181, &info);
  printf("Run ps now.\n");
  sleep(10);
printf("state is %ld\npid is %d\nparent_pid is %d\nyoungest_child_pid is %d\nyounger_sibling_pid is %d\nolder_sibling_pid is %d\n", info.state, info.pid, info.parent_pid, info.youngest_child_pid, info.younger_sibling_pid, info.older_sibling_pid);
  return 0;
}

/*
 * Function: test1()
 *
 * Description:
 *  This function tests for accuracy of PIDs between parents and children.
 */
int test1() {
  pid_t fork_result1, fork_result2;
  pid_t grandparent_pid = getpid();
  pid_t parent_pid = -1;
  pid_t grandchild_pid = -1;
  int waitstatus1, waitstatus2;
  if ((fork_result1 = fork()) == 0) {
    if ((fork_result2 = fork()) == 0) {
      // within grandchild
      sleep(2);
      exit(0);
    }
    else if (fork_result2 > 0) {
      // within parent
      parent_pid = getpid();
      grandchild_pid = fork_result2;
      struct prinfo grandparent_info, grandchild_info, parent_info;
      grandparent_info.pid = grandparent_pid;
      parent_info.pid = parent_pid;
      grandchild_info.pid = grandchild_pid;
      printf("Before our system call:\n");
      printf("grandparent's child = %d\n", grandparent_info.youngest_child_pid);
      printf("parent = %d\n", parent_info.pid);
      printf("grandchild's parent = %d\n\n", grandchild_info.parent_pid);
      syscall(181, &grandparent_info);
      syscall(181, &parent_info);
      syscall(181, &grandchild_info);
      int ret1 = (grandparent_info.youngest_child_pid == parent_info.pid && parent_info.pid == grandchild_info.parent_pid);
      int ret2 = (parent_info.parent_pid == grandparent_info.pid);
      int ret3 = (grandchild_info.pid == parent_info.youngest_child_pid);
      printf("grandparent's child = %d\n", grandparent_info.youngest_child_pid);
      printf("parent = %d\n", parent_info.pid);
      printf("grandchild's parent = %d\n\n", grandchild_info.parent_pid);
      if (waitpid(fork_result2, &waitstatus1, 0) == -1)
	return 0;
      return ret1 && ret2 && ret3;
    }
    else {
      printf("fork failed!\n");
      return 0;
    }
    exit(0);
  }
  else if (fork_result1 > 0) {
    // within grandparent
    if (waitpid(fork_result1, &waitstatus2, 0) == -1)
      return 0;
  }
  else {
    printf("fork failed!\n");
    return 0;
  }
  return 0;
}

int test2() {
  struct prinfo older, younger;
  pid_t fork_result1, fork_result2;
  int waitstatus;
  if ((fork_result1 = fork()) > 0) {
    if ((fork_result2 = fork()) > 0) {
      older.pid = fork_result1;
      younger.pid = fork_result2;
      syscall(181, &older);
      syscall(181, &younger);
      if (waitpid(fork_result1, &waitstatus, 0) == -1) return 0;
      if (waitpid(fork_result2, &waitstatus, 0) == -1) return 0;
    }
    else if (fork_result2 == 0) {
      sleep(1);
      exit(0);
    }
    else {
      printf("fork failed!\n");
      return 0;
    }
  }
  else if (fork_result1 == 0) {
    sleep(1);
    exit(0);
  }
  else {
    printf("fork failed!\n");
    return 0;
  }
  printf("in test2:\nyounger = %d\nyounger of older = %d\n", younger.pid, older.younger_sibling_pid);
  return (older.younger_sibling_pid == younger.pid) && (younger.older_sibling_pid == older.pid);
}

int test3() {
  int fd1 = open("~/Desktop/fd1.txt", O_RDWR | O_CREAT);
  int fd2 = open("~/Desktop/fd2.txt", O_RDWR | O_CREAT);
  struct prinfo info;
  info.pid = getpid();
  syscall(181, &info);
  printf("in test3: open fds = %lu should be 2\n", info.num_open_fds);
  int ret1 = (info.num_open_fds == 2);
  close(fd2);
  syscall(181, &info);
  printf("in test3: open fds = %lu should be 1\n", info.num_open_fds);
  int ret2 = (info.num_open_fds == 1);
  close(fd1);
  syscall(181, &info);
  printf("in test3: open fds = %lu should be 0\n", info.num_open_fds);
  return ret1 && ret2 && (info.num_open_fds == 0);
}
