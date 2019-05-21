#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

/**
 * Here I wanted to find out whether the stop in p02.c was because of
 * the delivery of a signal or because of a syscall. I used an
 * arbitrary signal instead (SIGFPE) and looked at the information
 * returned by waitpid. It does indeed stop because of the signal
 * (which I suppress by just throwing it away).
 *
 * Note that had the signal been one of SIGSTOP, SIGTSTP, SIGTTIN, or
 * SIGTTOU, then we wouldn't be able distinguish signal-delivery-stop
 * from group-stop without also calling ptrace(PTRACE_GETSIGINFO,
 * ...).
 */

int main() {
    pid_t child = fork();
    if (child == 0) {
        ptrace(PTRACE_TRACEME, 0, 0, 0);
        raise(SIGFPE);
        execl("/bin/ls", "ls", NULL);
    } else {
        int status;
        int pid = waitpid(child, &status, __WALL);
        if (pid > 0 && WIFSTOPPED(status)) {
            printf("Child is ptrace-stopped");
            if (WSTOPSIG(status) == SIGFPE) {
                printf(" (signal-delivery-stop, SIGFPE)\n");
            }
        }
        ptrace(PTRACE_CONT, child, 0, 0);
    }

    return 0;
}
