#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/user.h>
#include <sys/reg.h>

/**
 * This code is a slightly modified version of the first example in
 * https://www.linuxjournal.com/article/6100.
 *
 * It works as intended on my 64-bit system and prints the message in
 * the parent process. System call number 59 is `execve`.
 *
 * I believe that we know that it is a system call in the parent only
 * because we know that the child will execute the execl right after
 * its ptrace call. Theoretically, I believe that if it would receive
 * a signal between the call to ptrace and the call to execl, that
 * signal would instead be intercepted by the parent, and this program
 * would incorrectly report that a system call occurred.
 *
 * I don't know if it's feasible to time such a signal without
 * modifying the child. But a simpler test would be to just insert a
 * raise(SIGSTOP) after the ptrace call, just as it is described in
 * the man page of ptrace. My hypothesis is then that the `wait()` in
 * the parent will receive that SIGSTOP signal, and the
 * PTRACE_PEEKUSER will no longer apply to the syscall. I'm trying
 * that out in p02.c.
 */

int main() {
    pid_t child = fork();
    if (child == 0) {
        ptrace(PTRACE_TRACEME, 0, 0, 0);
        execl("/bin/ls", "ls", NULL);
    } else {
        wait(NULL);
        long orig_rax = ptrace(PTRACE_PEEKUSER, child, 8 * ORIG_RAX, 0);
        printf("The child made a system call %ld\n", orig_rax);
        ptrace(PTRACE_CONT, child, 0, 0);
    }

    return 0;
}
