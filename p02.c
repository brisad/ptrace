#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/user.h>
#include <sys/reg.h>
#include <signal.h>

/**
 * In this program the parent says "The child made a system call
 * 234". So while it no longer refers to the call to execl, the system
 * call number still refers to a system call, `tgkill`, which is
 * probably what was called by raise. I'm not sure if the program
 * stopped because of the syscall, or if it was because of the actual
 * signal and I'm just reading an old value.
 *
 * PTRACE_PEEKUSER does perhaps not give me the right information in
 * order to determine the cause of the stop. I think I might be able
 * to get that information from `wait()` or `waitpid()` instead.
 */

int main() {
    pid_t child = fork();
    if (child == 0) {
        ptrace(PTRACE_TRACEME, 0, 0, 0);
        raise(SIGSTOP);
        execl("/bin/ls", "ls", NULL);
    } else {
        wait(NULL);
        long orig_rax = ptrace(PTRACE_PEEKUSER, child, 8 * ORIG_RAX, 0);
        printf("The child made a system call %ld\n", orig_rax);
        ptrace(PTRACE_CONT, child, 0, 0);
    }

    return 0;
}
