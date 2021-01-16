#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/reg.h>
#include <sys/user.h>
#include <sys/syscall.h>   /* For SYS_write etc */
#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/reg.h>
#include <errno.h>



/* Index into an array of 8 byte longs returned from ptrace for
    location of the users' stored general purpose registers.  */
//
//# define R15    0
//# define R14    1
//# define R13    2
//# define R12    3
//# define RBP    4
//# define RBX    5
//# define R11    6
//# define R10    7
//# define R9 8
//# define R8 9
//# define RAX    10
//# define RCX    11
//# define RDX    12
//# define RSI    13
//# define RDI    14
//# define ORIG_RAX 15
//# define RIP    16
//# define CS 17
//# define EFLAGS 18
//# define RSP    19
//# define SS 20
//# define FS_BASE 21
//# define GS_BASE 22
//# define DS 23
//# define ES 24
//# define FS 25
//# define GS 26

///* Index into an array of 4 byte integers returned from ptrace for
// * location of the users' stored general purpose registers. */
//
//# define EBX 0
//# define ECX 1
//# define EDX 2
//# define ESI 3
//# define EDI 4
//# define EBP 5
//# define EAX 6
//# define DS 7
//# define ES 8
//# define FS 9
//# define GS 10
//# define ORIG_EAX 11
//# define EIP 12
//# define CS  13
//# define EFL 14
//# define UESP 15
//# define SS   16
//



int main() {
    const char* programname = "/home/student/Desktop/Atam4/Code-v1/a.out";
    int fd = open("out2.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);

    pid_t pid = fork();
    if(pid == 0) {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        printf("now call execl\n");
        execl(programname, programname, NULL);
        printf("we will not see this");
//        execl("/bin/ls", "ls", NULL);
//        _exit(0);  // TODO useless
    }

    unsigned long long prev_rdi = 0;
    unsigned long long prev_rsi = 0;
    unsigned long long prev_rpi = 0;
    struct user_regs_struct last_regs;
    int wait_status;
    int last;
    int counter = 5;
    wait(&wait_status);
    while (WIFSTOPPED(wait_status)) {
        /* Enter next system call */
        ptrace(PTRACE_SYSCALL, pid, 0, 0);
        wait(&wait_status);

        struct user_regs_struct regs;
        ptrace(PTRACE_GETREGS, pid, 0, &regs);

        /* Is this system call permitted? */
        int blocked = 0;
        if (regs.orig_rax == SYS_write) {
            blocked = 1;
            prev_rdi = regs.rdi;
            regs.rdi = fd;
            printf("this is syswrite - before \n");
            ptrace(PTRACE_SETREGS, pid, 0, &regs);
        }

        /* Run system call and stop on exit */
        ptrace(PTRACE_SYSCALL, pid, 0, 0);
        wait(&wait_status);

        if (blocked) {
            printf("this is syswrite - after \n");
            ptrace(PTRACE_GETREGS, pid, 0, &regs);
            regs.rdi = prev_rdi;
            ptrace(PTRACE_SETREGS, pid, 0, &regs);
            blocked = 0;
            counter--;
        }
    }
    close(fd);



    return 0;
}