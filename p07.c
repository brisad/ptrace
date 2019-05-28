#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/reg.h>
#include <sys/syscall.h>
#include <stdbool.h>
#include <signal.h>
#include <assert.h>

/**
 * A version of p06 which instead attaches to a running process and
 * reverses data passed to write.
 *
 * One improvement in this program is that we use the
 * PTRACE_O_TRACESYSGOOD option, which will cause WSTOPSIG return
 * (SIGTRAP | 0x80) on syscall-stop. That way we can easily
 * distinguish syscalls from regular signal deliveries without needing
 * to invoke PTRACE_GETSIGINFO.
 */

void reverse(char *s)
{
    char temp;
    char *t = s + (strlen(s) - 1);
    if (*t == '\n') t--;
    while (s < t) {
        temp = *s;
        *s++ = *t;
        *t-- = temp;
    }
}

union u {
    long val;
    char chars[8];
} data;

void getdata(pid_t child, long addr, char *str, int len)
{
    long end = addr + len;
    while (addr < end) {
        int n = end - addr;
        n = n > 8 ? 8 : n;
        data.val = ptrace(PTRACE_PEEKDATA, child, addr, 0);
        memcpy(str, data.chars, n);
        str += n;
        addr += n;
    }
    *str = '\0';
}

void putdata(pid_t child, long addr, char *str, int len)
{
    long end = addr + len;
    while (addr < end) {
        int n = end - addr;
        n = n > 8 ? 8 : n;
        memcpy(data.chars, str, n);
        ptrace(PTRACE_POKEDATA, child, addr, data.val);
        str += n;
        addr += n;
    }
}

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
    while (1) {
        ptrace(PTRACE_SYSCALL, pid, 0, 0);

        if (waitpid(pid, &status, __WALL) == -1) {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }

        if (WIFEXITED(status)) {
            printf("Tracee exited\n");
            break;
        }

        assert(WIFSTOPPED(status));
        if (WSTOPSIG(status) != (SIGTRAP | 0x80)) {
            printf("Ignoring signal %d ...\n", WSTOPSIG(status));
            continue;
        }

        long orig_rax = ptrace(PTRACE_PEEKUSER, pid, 8 * ORIG_RAX, 0);
        if (orig_rax == SYS_write) {
            if (!insyscall) {
                insyscall = true;
                struct user_regs_struct regs;
                ptrace(PTRACE_GETREGS, pid, 0, &regs);

                char *str = calloc(regs.rdx + 1, 1);
                getdata(pid, regs.rsi, str, regs.rdx);
                reverse(str);
                putdata(pid, regs.rsi, str, regs.rdx);
            } else {
                insyscall = false;
            }
        }
    }
    return 0;
}
