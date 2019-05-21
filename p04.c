#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>

/**
 * Since we're not using PTRACE_SYSCALL (or PTRACE_SYSEMU) when we
 * report the syscall in p01.c, we shouldn't enter a syscall-stop
 * state. So I wonder why the tracer stops. What it the real reason
 * for the stop?
 *
 * The following program gives us the answer. We get a SIGTRAP, and
 * with the help of PTRACE_GETSIGINFO we can determine that it is a
 * normal user-space delivered signal. Where does that one come from?
 *
 * Reading the ptrace manual, it turns out that execve system call is
 * specially handled by ptrace and we get a SIGTRAP for that
 * particular syscall. Other syscalls aren't handled like this, and if
 * we were to do other syscalls before execl, we wouldn't see them in
 * the tracer. Thus, the example program is a bit misleading as it
 * seems like it tries to log any syscalls, which is not the case.
 *
 * The SIGTRAP that is sent on execve is not without issues. See the
 * ptrace manual for details, but the recommended approach is to set
 * the PTRACE_O_TRACEEXEC option (or use PTRACE_SEIZE) and things will
 * be less ambiguous.
 */

int main() {
    pid_t child = fork();
    if (child == 0) {
        ptrace(PTRACE_TRACEME, 0, 0, 0);
        execl("/bin/ls", "ls", NULL);
    } else {
        int status;
        siginfo_t siginfo;
        int pid = waitpid(child, &status, __WALL);
        if (pid > 0 && WIFSTOPPED(status)) {
            assert(WSTOPSIG(status) == SIGTRAP);
            // We get a SIGTRAP, which mean different things in
            // different circumstances. Since we haven't used any
            // PTRACE_O_TRACE* options, we here need to distinguish
            // between a normal signal delivery and a syscall-stop;
            // with the help of PTRACE_GETSIGINFO. (Actually, since we
            // haven't used PTRACE_SYSCALL, we probably don't need to
            // this at all, but this is for me to learn).
            ptrace(PTRACE_GETSIGINFO, child, 0, &siginfo);
            if (siginfo.si_code <= 0) {
                printf("Sent with user-space action\n");
            } else if (siginfo.si_code == SI_KERNEL) {
                printf("Sent by kernel\n");
            } else if (siginfo.si_code == SIGTRAP ||
                       siginfo.si_code == (SIGTRAP | 0x80)) {
                printf("Syscall-stop\n");
            }
        }
        ptrace(PTRACE_CONT, child, 0, 0);
    }

    return 0;
}
