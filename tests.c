#include <sys/types.h>
#include <linux/unistd.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <fcntl.h>

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
  sleep(60);
printf("state is %ld\npid is %d\nparent_pid is %d\nyoungest_child_pid is %d\nyounger_sibling_pid is %d\nolder_sibling_pid is %d\n", info.state, info.pid, info.parent_pid, info.youngest_child_pid, info.younger_sibling_pid, info.older_sibling_pid);
  return 0;
}

int test1() {
  struct prinfo info;
  pid_t fork_result1, fork_result2;
  pid_t grandparent_pid = getpid();
  pid_t parent_pid1 = -1;
  pid_t parent_pid2 = -1;
  pid_t grandchild_pid1 = -1;
  pid_t grandchild_pid2 = -1;
  int waitstatus1, waitstatus2;
  if ((fork_result1 = fork()) == 0) {
    if ((fork_result2 = fork()) == 0) {
      // within grandchild
      grandchild_pid1 = getpid();
      exit(0);
    }
    else if (fork_result2 > 0) {
      // within parent
      parent_pid1 = getpid();
      grandchild_pid2 = fork_result2;
      if (waitpid(grandchild_pid2, &waitstatus1, NULL) == -1)
	return 0;
    }
    else {
      printf("fork failed!\n");
      return 0;
    }
    exit(0);
  }
  else if (fork_result2 > 0) {
    // within grandparent
    parent_pid2 = fork_result1;
    if (waitpid(parent_pid2, &waitstatus2, NULL) == -1)
      return 0;
  }
  else {
    printf("fork failed!\n");
    return 0;
  }
  if (grandchild_pid1 != grandchild_pid2) return 0;
  if (parent_pid1 != parent_pid2) return 0;
  struct prinfo grandparent_info, grandchild_info, parent_info;
  grandparent_info.pid = grandparent_pid;
  parent_info.pid = parent_pid1;
  grandchild_info.pid = grandchild_pid1;
  syscall(181, &grandparent_info);
  syscall(181, &parent_info);
  syscall(181, &grandchild_info);
  int ret1 = (grandparent_info.youngest_child_pid == parent_info.pid == grandchild_info.parent_pid);
  int ret2 = (parent_info.parent_pid == grandparent_info.pid);
  int ret3 = (grandchild_info.pid == parent_info.youngest_child_pid);
  return ret1 && ret2 && ret3;
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
      if (waitpid(fork_result1, &waitstatus, NULL) == -1) return 0;
      if (waitpid(fork_result2, &waitstatus, NULL) == -1) return 0;
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
  return (older.younger_sibling_pid == younger.pid) && (younger.older_sibling_pid == older.pid);
}

int test3() {
  int fd1 = open("~/Desktop/fd1.txt", O_RDWR | O_CREAT);
  int fd2 = open("~/Desktop/fd2.txt", O_RDWR | O_CREAT);
  struct prinfo info;
  info.pid = getpid();
  syscall(181, &info);
  int ret1 = (info.num_open_fds == 2);
  close(fd2);
  syscall(181, &info);
  int ret2 = (info.num_open_fds == 1);
  close(fd1);
  syscall(181, &info);
  return ret1 && ret2 && (info.num_open_fds == 0);
}
