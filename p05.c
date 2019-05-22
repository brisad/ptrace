#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/user.h>
#include <sys/reg.h>
#include <sys/syscall.h>
#include <stdbool.h>

/**
 * The following code is based on the third example in
 * https://www.linuxjournal.com/article/6100.
 *
 * If WIFEXITED(status) is true, the child has terminated and we break
 * from the loop. But we cannot assume that the child always reports
 * WIFEXITED(status) when it dies. There are cases when that does not
 * occur.
 *
 * The tracer cannot distinguish between a syscall-enter-stop and
 * syscall-exit-stop, and we therefore need to track which one we
 * expect. However, there are complexities here which are ignored but
 * really should be taken into account, but that is described more in
 * depth in the ptrace manual.
 */

int main(void) {
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
                    // Syscall entry
                    insyscall = true;
                    struct user_regs_struct regs;
                    ptrace(PTRACE_GETREGS, child, 0, &regs);
                    printf("Write called with %lld, %lld, %lld\n",
                           regs.rdi, regs.rsi, regs.rdx);
                } else {
                    long rax = ptrace(PTRACE_PEEKUSER, child, 8 * RAX, 0);
                    printf("Write returned with %ld\n", rax);
                    insyscall = false;
                }
            }
            ptrace(PTRACE_SYSCALL, child, 0, 0);
        }
    }

    return 0;
}
