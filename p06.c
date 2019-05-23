#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/user.h>
#include <sys/reg.h>
#include <sys/syscall.h>
#include <stdbool.h>

/**
 * A cleaned-up version of the fourth example in
 * https://www.linuxjournal.com/article/6100.
 *
 * It reverses data sent do the write system call.
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

int main(void)
{
    pid_t child = fork();
    if (child == 0) {
        ptrace(PTRACE_TRACEME, 0, 0, 0);
        execl("/bin/ls", "ls", NULL);
    } else {
        int status;
        bool insyscall = false;
        while (1) {
            wait(&status);
            if (WIFEXITED(status)) {
                break;
            }
            long orig_rax = ptrace(PTRACE_PEEKUSER, child, 8 * ORIG_RAX, 0);
            if (orig_rax == SYS_write) {
                if (!insyscall) {
                    insyscall = true;
                    struct user_regs_struct regs;
                    ptrace(PTRACE_GETREGS, child, 0, &regs);

                    char *str = calloc(regs.rdx + 1, 1);
                    getdata(child, regs.rsi, str, regs.rdx);
                    reverse(str);
                    putdata(child, regs.rsi, str, regs.rdx);
                } else {
                    insyscall = false;
                }
            }
            ptrace(PTRACE_SYSCALL, child, 0, 0);
        }
    }

    return 0;
}
