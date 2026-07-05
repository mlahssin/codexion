#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>


int main()
{
    //pid = 0 means the child process
    int pid = fork();
    if (pid == -1)
        return 1;
    printf("process id %d\n", getpid());
    if (pid != 0)
        wait(NULL);
    return 0;
}