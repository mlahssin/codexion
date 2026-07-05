# include <stdio.h>
# include <time.h>
#include <stdlib.h>
#include <pthread.h>



int prime[10] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29};


void *print_unique(void *arg)
{
    printf("%d\n", *(int *)arg);
    return NULL;
}


int main()
{
    pthread_t th[10];
    for (int i = 0; i < 10; i++)
    {
        pthread_create(th + i, NULL, print_unique, &prime[i]);
    }

    for (int i = 0; i < 10; i++)
    {
        pthread_join(th[i], NULL);
    }
}