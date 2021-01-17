#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <fcntl.h>


int main() {
    const char* programname = "/home/student/Desktop/Atam4/Code-v1/my_program";
    int redirect_file = open("out2.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);

    pid_t pid = fork();
    if(pid == 0) {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        printf("now call execl\n");
        execl(programname, programname, NULL);
        printf("we will not see this");
//        _exit(0);  // TODO useless
    }


    unsigned long long prev_rdi = 0;
    unsigned long long prev_rip = 0;
    int wait_status = 1;
    struct user_regs_struct regs;
    ptrace(PTRACE_GETREGS, pid, 0, &regs);
    unsigned long address = 0x0000000000400109;
    int print_to_screen = 1;


    wait(&wait_status);
    while (WIFSTOPPED(wait_status) && regs.rip != address) {
        printf("<< rip is %llx \n", regs.rip);
        prev_rip = regs.rip;
        ptrace(PTRACE_GETREGS, pid, 0, &regs);
//        printf("rip is %llx \n", regs.rip);
        ptrace(PTRACE_SINGLESTEP, pid, NULL, NULL);
        wait(&wait_status);
    }
    printf("prev rip is %lx", prev_rip);

    if (regs.rip != address) {
        close(redirect_file);
        return 0;
    }
//    unsigned long bp_addr = prev_rip+1;
//    printf("BP address is %lx \n", bp_addr);
//    unsigned long data = ptrace(PTRACE_PEEKTEXT, pid, (void*)bp_addr, NULL);
//
//    unsigned long data_trap = (data & 0xFFFFFF00) | 0xCC;
//    ptrace(PTRACE_POKETEXT, pid, (void*)bp_addr, (void*)data_trap);

//    wait(&wait_status); // TODO useless?
    while (WIFSTOPPED(wait_status) && prev_rip+1 != regs.rip) {
        /* Enter next system call */

        ptrace(PTRACE_SYSCALL, pid, 0, 0);
        wait(&wait_status);

        struct user_regs_struct regs;
        ptrace(PTRACE_GETREGS, pid, 0, &regs);
        printf("rip is %llx \n", regs.rip);
//        if (regs.rip == bp_addr) {
//            printf("BREACK LOOP\n");
//            break;
//        }

        int blocked = 0;
        if (regs.orig_rax == SYS_write) {
            blocked = 1;
            prev_rdi = regs.rdi;
            regs.rdi = redirect_file;
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
        }
    }

    close(redirect_file);

    /* Let the child run to the breakpoint and wait for it to reach it */
    ptrace(PTRACE_CONT, pid, NULL, NULL);
    wait(&wait_status);


    return 0;
}