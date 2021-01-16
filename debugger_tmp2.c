#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/reg.h>
#include <sys/user.h>
#include <sys/syscall.h>   /* For SYS_write etc */
#include <stdio.h>


/* Index into an array of 8 byte longs returned from ptrace for
    location of the users' stored general purpose registers.  */

# define R15    0
# define R14    1
# define R13    2
# define R12    3
# define RBP    4
# define RBX    5
# define R11    6
# define R10    7
# define R9 8
# define R8 9
# define RAX    10
# define RCX    11
# define RDX    12
# define RSI    13
# define RDI    14
# define ORIG_RAX 15
# define RIP    16
# define CS 17
# define EFLAGS 18
# define RSP    19
# define SS 20
# define FS_BASE 21
# define GS_BASE 22
# define DS 23
# define ES 24
# define FS 25
# define GS 26

/* Index into an array of 4 byte integers returned from ptrace for
 * location of the users' stored general purpose registers. */

# define EBX 0
# define ECX 1
# define EDX 2
# define ESI 3
# define EDI 4
# define EBP 5
# define EAX 6
# define DS 7
# define ES 8
# define FS 9
# define GS 10
# define ORIG_EAX 11
# define EIP 12
# define CS  13
# define EFL 14
# define UESP 15
# define SS   16




int main() {
    unsigned long long orig_rdi = 0;
    int status;
    int is_syswrite = 0;
    pid_t pid = fork();
    int counter = 0;

    if(pid == 0) {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
//        execl("/home/student/Desktop/Atam4/Code-v1/dummy_main.c", "", NULL);
        execl("/home/student/Desktop/Atam4/Code-v1/a.out", "", NULL);
//        execl("/bin/ls", "ls", NULL);
        _exit(0);
    }

    wait(&status);
    while ((WIFSTOPPED(status))) {
        struct user_regs_struct regs;
        ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
        ptrace(PTRACE_GETREGS, pid, NULL, &regs);
        orig_rdi = regs.rdi;
        printf("rax is %llu \n", regs.rax);
        printf("counter is %d \n", counter++);
        if (regs.rax == SYS_write) {
            printf("try to catch forked son syswrite\n");
            regs.rdi = "/outputs_t/out1.txt";
            is_syswrite = 1;
            ptrace(PTRACE_SETREGS, pid, NULL, &regs);
        }
        ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
        if (is_syswrite) {
            ptrace(PTRACE_GETREGS, pid, NULL, &regs);
            regs.rdi = orig_rdi;
            ptrace(PTRACE_SETREGS, pid, NULL, &regs);
            is_syswrite = 0;
        }
    }

    return 0;
}