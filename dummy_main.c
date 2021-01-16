#include <stdio.h>
//#include <syscall.h>
//#include <sys/types.h>
//#include <unistd.h>
//#include <linux/ptrace.h>
//#include <sys/ptrace.h>
//#include <sys/types.h>
//#include <sys/wait.h>
//#include <unistd.h>


int main() {
    printf("hello %d \n", 0);
    printf("check1\n");
    printf("check2\n");
    for (int i = 1; i < 4; ++i) {
        printf("hello %d \n", i);
    }
    return 0;

}