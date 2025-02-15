#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>


int main(int argc, char * argv[]) {
    if (argc < 5) {
        // TODO print something about it?
        return 0;
    }

//    for (int i = 0; i < argc; ++i) { // TODO only for testing
//        printf("%d ) %s \n", i, argv[i]);
//    }

    const char* function_address = argv[1];
    const char* flag_screen_print = argv[2];
    const char* output_file_name = argv[3];
    const char* program_name = "/home/student/Desktop/Atam4/Code-v1/my_program";
//    const char* program_name = argv[4]; // TODO. does not work when given this param. but work with the prev line ^
    int redirect_file = open(output_file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    pid_t pid = fork();
    if(pid == 0) {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        if (argc == 5) {
            execl(program_name, program_name, NULL);
        }
        else {
            execl(program_name, program_name, argv[4]);
        }
        _exit(0);  // uses only if execl fails // TODO print something about execl error?
    }
    else if (pid < 0) {
        perror("fork failed");
        return 0;
    }

    unsigned long long prev_rdi = 0;
    unsigned long long prev_rip = 0;
    int wait_status = 1;
    struct user_regs_struct regs;
    ptrace(PTRACE_GETREGS, pid, 0, &regs);
    unsigned long address = (int) strtol(function_address, (char **)NULL, 16);
    int print_to_screen = 1;


    wait(&wait_status);
    while (WIFSTOPPED(wait_status) && regs.rip != address) {
        prev_rip = regs.rip;
        ptrace(PTRACE_GETREGS, pid, 0, &regs);
        ptrace(PTRACE_SINGLESTEP, pid, NULL, NULL);
        wait(&wait_status);
    }
    printf("prev rip is %llx \n", prev_rip);
    printf("curr rip is %llx \n", regs.rip);

    if (regs.rip != address) { // did not find function address in the given program (finish the program)
        close(redirect_file);
        return 0;
    }

    unsigned long addr = prev_rip+5; // TODO wtf + 5??? i tried also 0,1,2,3,4
    printf("BP address is %lx \n", addr);

    unsigned long data = ptrace(PTRACE_PEEKTEXT, pid, (void*)addr, NULL);
    printf("DBG: Original data at 0x%lx: 0x%lx\n", addr, data);

    /* Write the trap instruction 'int 3' into the address */
    unsigned long data_trap = (data & 0xFFFFFF00) | 0xCC;
    ptrace(PTRACE_POKETEXT, pid, (void*)addr, (void*)data_trap);


    while (WIFSTOPPED(wait_status) && addr+1 != regs.rip) {
        /* Enter next system call */

        ptrace(PTRACE_SYSCALL, pid, 0, 0);
        wait(&wait_status);

        struct user_regs_struct regs;
        ptrace(PTRACE_GETREGS, pid, 0, &regs);
        printf("rip is %llx \n", regs.rip);
        if (regs.rip == addr+1) {
            printf("BREACK LOOP\n");
            break;
        }

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
    ptrace(PTRACE_GETREGS, pid, 0, &regs);
    printf("DBG: Child stopped at RIP = 0x%llx\n", regs.rip);

    /* Remove the breakpoint by restoring the previous data and set rdx = 5 */
    ptrace(PTRACE_POKETEXT, pid, (void*)addr, (void*)data);
    regs.rip -= 1;
    ptrace(PTRACE_SETREGS, pid, 0, &regs);

    /* The child can continue running now */
    ptrace(PTRACE_CONT, pid, 0, 0);

    wait(&wait_status);

    return 0;
}