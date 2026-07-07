#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int x = 2;
void *routine1()
{
    x++;
    sleep(2);
    printf("process id : %d\n", getpid());
    printf("value of x: %d\n", x);
    return NULL;
}

void *routine2()
{
    sleep(2);
    printf("process id : %d\n", getpid());
    printf("value of x: %d\n", x);
    return NULL;
}


// Threads created with pthread_create() share the same address space
// of the process. They can access and modify the same global variables,
// heap memory, and open files. Changes made by one thread are visible
// to the other threads.
//
// Each thread has its own:
// - Stack (local variables)
// - CPU registers
// - Program counter

int main()
{
    pthread_t t1, t2;

    pthread_create(&t1, NULL, &routine1, NULL);

    pthread_create(&t2, NULL, &routine2, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
}