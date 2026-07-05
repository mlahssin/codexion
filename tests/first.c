#include <pthread.h>
#include <stdio.h>
# include <unistd.h>

int x = 2;

void*    my_turn(void *arg)
{
    while(1)
    {
        x += 3;
        sleep(1);
        // printf("first id %d\n", getpid());
        printf("first x %d\n", x);
    }
    return NULL;
}

void*    your_turn(void *arg)
{
    while(1)
    {
        sleep(1);
        // printf("second id %d\n", getpid());
        printf("second x %d\n", x);
    }
    return NULL;
}

int main()
{
    pthread_t t1;
    pthread_t t2;

    printf("the main thread :%d\n", getpid());
    pthread_create(&t1, NULL, my_turn, NULL);
    pthread_create(&t2, NULL, your_turn, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);


    // my_turn();
    // your_turn();
}

