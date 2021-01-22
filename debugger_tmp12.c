#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char * argv[]) {
    if (argc < 5) {
        // TODO print something about it?
        return 0;
    }

    const char* function_address = argv[1];
    const char* flag_screen_print = argv[2];
    const char* output_file_name = argv[3];
    const char* program_name = argv[4];

    int redirect_file = open(output_file_name, O_CREAT|O_WRONLY|O_APPEND | O_TRUNC, 0777);  // TODO 0644 ? 0666 ?
    int redirect_file_for_debugger = open(output_file_name, O_WRONLY|O_APPEND, 0777);

    pid_t pid = fork();
    if(pid == 0) {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        execv(program_name, argv[5]);  // TODO double check this syntax
        _exit(0);  // uses only if execl fails // TODO print something about execl error?

//        if (argc == 5) {
//            execl(program_name, argv[5], argc-5);
//            execl(program_name, program_name, NULL);
//        }
//        else {
//            execl(program_name, argv[5], argc-5);
//        }
//        _exit(0);  // uses only if execl fails // TODO print something about execl error?
    }
    else if (pid < 0) {
        perror("fork failed");
        exit(1);
    }

    unsigned long foo_addr = (int) strtol(function_address, (char **)NULL, 16);
    struct user_regs_struct regs;
    int wait_status = 1;

    wait(&wait_status);
    // firstly, setting a BP at the start of foo func
    if (ptrace(PTRACE_GETREGS, pid, 0, &regs) == -1) {
        close(redirect_file);
        close(redirect_file_for_debugger);
        perror("ptrace failed");
        exit(1);
    }
    unsigned long foo_data = ptrace(PTRACE_PEEKTEXT, pid, (void*)foo_addr, NULL);
    unsigned long data_trap = (foo_data & 0xFFFFFF00) | 0xCC;
    if (ptrace(PTRACE_POKETEXT, pid, (void*)foo_addr, (void*)data_trap) == -1 ||
    ptrace(PTRACE_CONT, pid, NULL, NULL) == -1) {exit(1);} // and now we start the run of the program
    wait(&wait_status);

    if (ptrace(PTRACE_GETREGS, pid, 0, &regs) == -1) {exit(1);}
    while(WIFSTOPPED(wait_status) && regs.rip == foo_addr+1) {
        // remove BP from foo and restoring its first instruction (who was overwrite by int3 for the BP)
        if (ptrace(PTRACE_POKETEXT, pid, (void*)foo_addr, (void*)foo_data) == -1) {exit(1);}
        regs.rip -= 1; // go one instruction back to start from foo start
        if (ptrace(PTRACE_SETREGS, pid, 0, &regs) == -1) {exit(1);}

        // adding a BP at the returning address from foo (assume its found at the top of the stack)
        unsigned long bp_addr = ptrace(PTRACE_PEEKTEXT, pid, (void*)regs.rsp, NULL);
        unsigned long bp_data = ptrace(PTRACE_PEEKTEXT, pid, (void*)bp_addr, NULL);
        data_trap = (bp_data & 0xFFFFFF00) | 0xCC;
        if (ptrace(PTRACE_POKETEXT, pid, (void*)bp_addr, (void*)data_trap) == -1) {exit(1);}

        int print_to_screen = 1;
        while (WIFSTOPPED(wait_status) && bp_addr+1 != regs.rip) { // now run foo from syscall to syscall
            if (ptrace(PTRACE_SYSCALL, pid, 0, 0) == -1) {exit(1);}
            wait(&wait_status);

            struct user_regs_struct backup_regs;
            if (ptrace(PTRACE_GETREGS, pid, 0, &regs) == -1) {exit(1);}
            if (regs.rip == bp_addr+1) {
                break; // the debugger encounter the returning address BP, so out of the foo reading section
            }

            int is_syswrite = 0;
            if (regs.orig_rax == SYS_write) { // initiate special treatment only if this is syswrite
                is_syswrite = 1;
                backup_regs = regs;
                if (print_to_screen == 1) {
                    write(redirect_file_for_debugger, "PRF:: ", 6);
                    regs.rdi = redirect_file;
                }
                if (ptrace(PTRACE_SETREGS, pid, 0, &regs) == -1) {exit(1);}
            }

            if (ptrace(PTRACE_SYSCALL, pid, 0, 0) == -1) {exit(1);}
            wait(&wait_status);

            if (is_syswrite) {
                if (ptrace(PTRACE_GETREGS, pid, 0, &regs) == -1) {exit(1);}
                regs.rdi = backup_regs.rdi;
                if (flag_screen_print[0] == 'c') {
                    if (print_to_screen != 0) {
                        backup_regs.rip -= 2; // TODO why 2??!
                        regs = backup_regs;
                        regs.rax = 1; // TODO almost holly bug
                        print_to_screen = 0;
                    }
                    else {
                        ++print_to_screen;
                    }
                }
                if (ptrace(PTRACE_SETREGS, pid, 0, &regs) == -1) {exit(1);}
            }
        }

        if (ptrace(PTRACE_GETREGS, pid, 0, &regs) == -1) {exit(1);}

        // remove the BP from the returning address
        if (ptrace(PTRACE_POKETEXT, pid, (void*)bp_addr, (void*)bp_data) == -1) {exit(1);}
        regs.rip -= 1;
        if (ptrace(PTRACE_SETREGS, pid, 0, &regs) == -1) {exit(1);}


        // setting a BP at the start of foo func AGAIN, in case it will be called once more
        if (ptrace(PTRACE_GETREGS, pid, 0, &regs) == -1) {exit(1);}
        unsigned long foo_data = ptrace(PTRACE_PEEKTEXT, pid, (void*)foo_addr, NULL);
        unsigned long data_trap = (foo_data & 0xFFFFFF00) | 0xCC;
        if (ptrace(PTRACE_POKETEXT, pid, (void*)foo_addr, (void*)data_trap) == -1) {exit(1);}
        if (ptrace(PTRACE_CONT, pid, NULL, NULL) == -1) {exit(1);} // and now continue the run of the program
        /* if we will encounter foo BP (again) - we will loop again
         * else we will get to the end of debugging program and than this program will finish also
        */
         wait(&wait_status);

        if (ptrace(PTRACE_GETREGS, pid, 0, &regs) == -1) {exit(1);}
    }

    close(redirect_file);
    close(redirect_file_for_debugger);
    return 0;
}
