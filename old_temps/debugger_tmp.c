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




int main()
{   pid_t pid;
    long orig_eax, eax;
    long orig_rdi = 0;
    long params[3];
    int status;
    int insyscall = 0;
    pid = fork();
    if(pid == 0) {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        execl("/bin/ls", "ls", NULL);
    }
    else {
        while(1) {
            wait(&status);
            if(WIFEXITED(status))
                break;
            struct user_regs_struct regs;

            ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
            ptrace(PTRACE_GETREGS, pid, NULL, &regs);
            if (regs.rax == SYS_write) {
                orig_rdi = regs.rdi;
                regs.rdi = "outputs_t/out1.txt";
                ptrace(PTRACE_SETREGS, pid, NULL, &regs);
            }

            ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
            wait(&status);
            ptrace(PTRACE_GETREGS, pid, NULL, &regs);
            regs.rdi = orig_rdi;
            ptrace(PTRACE_SETREGS, pid, NULL, &regs);
            ptrace(PTRACE_CONT, pid, NULL, NULL);
            wait(&status);
//
//            orig_eax = ptrace(PTRACE_GETREGS,
//                              child, 4 * ORIG_EAX, NULL);
//            if(orig_eax == SYS_write) {
//                if(insyscall == 0) {
//                    /* Syscall entry */
//                    insyscall = 1;
//                    params[0] = ptrace(PTRACE_PEEKUSER,
//                                       child, 4 * EBX,
//                                       NULL);
//                    params[1] = ptrace(PTRACE_PEEKUSER,
//                                       child, 4 * ECX,
//                                       NULL);
//                    params[2] = ptrace(PTRACE_PEEKUSER,
//                                       child, 4 * EDX,
//                                       NULL);
//                    printf("Write called with "
//                           "%ld, %ld, %ld\n",
//                           params[0], params[1],
//                           params[2]);
//                }
//                else { /* Syscall exit */
//                    eax = ptrace(PTRACE_PEEKUSER,
//                                 child, 4 * EAX, NULL);
//                    printf("Write returned "
//                           "with %ld\n", eax);
//                    insyscall = 0;
//                }
//            }
//            ptrace(PTRACE_SYSCALL,
//                   child, NULL, NULL);
        }
    }
    return 0;
}

//#include <stdio.h>
//#include <syscall.h>
//#include <sys/types.h>
//#include <unistd.h>
//#include <sys/ptrace.h>
//#include <sys/types.h>
//#include <sys/wait.h>
//#include <unistd.h>
//#include <stdlib.h>
//
//
//int foo () {
//    printf("Helloworld");
//
//    return 0;
//}
//
//
//
//int main() {
//
//    pid_t pid = fork();
//    if (!pid) {
//
//        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
////        if (execv("/bin/bash", (char * const*) proc_args) == -1) {
//        execl("/dummy_main.c", "/dummy_main.c", NULL);
////        execv("/dummy_main.c", (char *const*)0);
//        printf("after exe");
//        exit(0);
//    }
//    int counter = 0;
//
//    int wait_status;
//    wait(&wait_status);
//    while (WIFSTOPPED(wait_status)) {
//        counter++;
//        ptrace(PTRACE_SINGLESTEP, pid, NULL, NULL);
//
//        wait(&wait_status);
//    }
//    printf("FINISH \n");
//    printf("%d", counter);
//    return 0;
//}
