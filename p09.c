#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/reg.h>
#include <stdbool.h>
#include <signal.h>
#include <assert.h>

/**
 * Prints executed syscalls, on x86_64.
 *
 * This is p08.c but where signals are delivered to the tracee and not
 * suppressed.
 *
 * When a stopped tracee is woken up again with SIGCONT, the tracer
 * will get a PTRACE_EVENT_STOP even though it is not a
 * group-stop. This is undocumented in the manual page for ptrace but
 * luckily I found this post which explains it:
 * https://stackoverflow.com/questions/49354408/why-does-a-sigtrap-ptrace-event-stop-occur-when-the-tracee-receives-sigcont
 */

char *syscalls[] =
    {
     "read", "write", "open", "close", "stat", "fstat", "lstat", "poll",
     "lseek", "mmap", "mprotect", "munmap", "brk", "rt_sigaction",
     "rt_sigprocmask", "rt_sigreturn", "ioctl", "pread64", "pwrite64", "readv",
     "writev", "access", "pipe", "select", "sched_yield", "mremap", "msync",
     "mincore", "madvise", "shmget", "shmat", "shmctl", "dup", "dup2", "pause",
     "nanosleep", "getitimer", "alarm", "setitimer", "getpid", "sendfile",
     "socket", "connect", "accept", "sendto", "recvfrom", "sendmsg", "recvmsg",
     "shutdown", "bind", "listen", "getsockname", "getpeername", "socketpair",
     "setsockopt", "getsockopt", "clone", "fork", "vfork", "execve", "exit",
     "wait4", "kill", "uname", "semget", "semop", "semctl", "shmdt", "msgget",
     "msgsnd", "msgrcv", "msgctl", "fcntl", "flock", "fsync", "fdatasync",
     "truncate", "ftruncate", "getdents", "getcwd", "chdir", "fchdir", "rename",
     "mkdir", "rmdir", "creat", "link", "unlink", "symlink", "readlink",
     "chmod", "fchmod", "chown", "fchown", "lchown", "umask", "gettimeofday",
     "getrlimit", "getrusage", "sysinfo", "times", "ptrace", "getuid", "syslog",
     "getgid", "setuid", "setgid", "geteuid", "getegid", "setpgid", "getppid",
     "getpgrp", "setsid", "setreuid", "setregid", "getgroups", "setgroups",
     "setresuid", "getresuid", "setresgid", "getresgid", "getpgid", "setfsuid",
     "setfsgid", "getsid", "capget", "capset", "rt_sigpending",
     "rt_sigtimedwait", "rt_sigqueueinfo", "rt_sigsuspend", "sigaltstack",
     "utime", "mknod", "uselib", "personality", "ustat", "statfs", "fstatfs",
     "sysfs", "getpriority", "setpriority", "sched_setparam", "sched_getparam",
     "sched_setscheduler", "sched_getscheduler", "sched_get_priority_max",
     "sched_get_priority_min", "sched_rr_get_interval", "mlock", "munlock",
     "mlockall", "munlockall", "vhangup", "modify_ldt", "pivot_root", "_sysctl",
     "prctl", "arch_prctl", "adjtimex", "setrlimit", "chroot", "sync", "acct",
     "settimeofday", "mount", "umount2", "swapon", "swapoff", "reboot",
     "sethostname", "setdomainname", "iopl", "ioperm", "create_module",
     "init_module", "delete_module", "get_kernel_syms", "query_module",
     "quotactl", "nfsservctl", "getpmsg", "putpmsg", "afs_syscall", "tuxcall",
     "security", "gettid", "readahead", "setxattr", "lsetxattr", "fsetxattr",
     "getxattr", "lgetxattr", "fgetxattr", "listxattr", "llistxattr",
     "flistxattr", "removexattr", "lremovexattr", "fremovexattr", "tkill",
     "time", "futex", "sched_setaffinity", "sched_getaffinity",
     "set_thread_area", "io_setup", "io_destroy", "io_getevents", "io_submit",
     "io_cancel", "get_thread_area", "lookup_dcookie", "epoll_create",
     "epoll_ctl_old", "epoll_wait_old", "remap_file_pages", "getdents64",
     "set_tid_address", "restart_syscall", "semtimedop", "fadvise64",
     "timer_create", "timer_settime", "timer_gettime", "timer_getoverrun",
     "timer_delete", "clock_settime", "clock_gettime", "clock_getres",
     "clock_nanosleep", "exit_group", "epoll_wait", "epoll_ctl", "tgkill",
     "utimes", "vserver", "mbind", "set_mempolicy", "get_mempolicy", "mq_open",
     "mq_unlink", "mq_timedsend", "mq_timedreceive", "mq_notify",
     "mq_getsetattr", "kexec_load", "waitid", "add_key", "request_key",
     "keyctl", "ioprio_set", "ioprio_get", "inotify_init", "inotify_add_watch",
     "inotify_rm_watch", "migrate_pages", "openat", "mkdirat", "mknodat",
     "fchownat", "futimesat", "newfstatat", "unlinkat", "renameat", "linkat",
     "symlinkat", "readlinkat", "fchmodat", "faccessat", "pselect6", "ppoll",
     "unshare", "set_robust_list", "get_robust_list", "splice", "tee",
     "sync_file_range", "vmsplice", "move_pages", "utimensat", "epoll_pwait",
     "signalfd", "timerfd_create", "eventfd", "fallocate", "timerfd_settime",
     "timerfd_gettime", "accept4", "signalfd4", "eventfd2", "epoll_create1",
     "dup3", "pipe2", "inotify_init1", "preadv", "pwritev", "rt_tgsigqueueinfo",
     "perf_event_open", "recvmmsg", "fanotify_init", "fanotify_mark",
     "prlimit64", "name_to_handle_at", "open_by_handle_at", "clock_adjtime",
     "syncfs", "sendmmsg", "setns", "getcpu", "process_vm_readv",
     "process_vm_writev", "kcmp", "finit_module", "sched_setattr",
     "sched_getattr", "renameat2", "seccomp", "getrandom", "memfd_create",
     "kexec_file_load", "bpf", "execveat", "userfaultfd", "membarrier",
     "mlock2", "copy_file_range", "preadv2", "pwritev2", "pkey_mprotect",
     "pkey_alloc", "pkey_free", "statx", "io_pgetevents", "rseq"
    };

void usage(const char *name)
{
    fprintf(stderr, "Usage: %s <pid>\n", name);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        usage(argv[0]);
    }

    pid_t pid = atoi(argv[1]);
    if (pid <= 0) {
        usage(argv[0]);
    }

    if (ptrace(PTRACE_SEIZE, pid, 0, PTRACE_O_TRACESYSGOOD) == -1) {
        perror("PTRACE_SEIZE");
        exit(EXIT_FAILURE);
    }

    if (ptrace(PTRACE_INTERRUPT, pid, 0, 0) == -1) {
        perror("PTRACE_INTERRUPT");
        exit(EXIT_FAILURE);
    }

    int status;
    if (waitpid(pid, &status, __WALL) == -1) {
        perror("waitpid");
        exit(EXIT_FAILURE);
    }

    assert(WIFSTOPPED(status));
    assert(WSTOPSIG(status) == SIGTRAP);
    assert(status >> 16 == PTRACE_EVENT_STOP);
    printf("Successfully attached to and interrupted process\n");

    bool insyscall = false;
    int last_signal = 0;
    while (1) {
        if (ptrace(PTRACE_SYSCALL, pid, 0, last_signal) == -1) {
            perror("PTRACE_SYSCALL");
            exit(EXIT_FAILURE);
        }
        last_signal = 0;

wait:
        if (waitpid(pid, &status, __WALL) == -1) {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }

        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            printf("Tracee terminated\n");
            break;
        }

        if (status >> 8 == (SIGTRAP | PTRACE_EVENT_STOP << 8)) {
            printf("Continuing...\n");
            continue;
        }
        else if (status >> 16 == PTRACE_EVENT_STOP) {
            printf("Group stop... Signal = %d\n", WSTOPSIG(status));
            if (ptrace(PTRACE_LISTEN, pid, 0, 0) == -1) {
                perror("PTRACE_LISTEN");
                exit(EXIT_FAILURE);
            }
            goto wait;
        }

        assert(WIFSTOPPED(status));
        if (WSTOPSIG(status) != (SIGTRAP | 0x80)) {
            printf("Signal %d delivered\n", WSTOPSIG(status));
            last_signal = WSTOPSIG(status);
            continue;
        }

        long orig_rax = ptrace(PTRACE_PEEKUSER, pid, 8 * ORIG_RAX, 0);

        if (!insyscall) {
            insyscall = true;
            printf("%s(", syscalls[orig_rax]);
            fflush(stdout);
        } else {
            printf(")\n");
            insyscall = false;
        }
    }
    return 0;
}
