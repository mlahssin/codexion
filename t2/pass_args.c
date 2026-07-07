#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
# include <time.h>



int primes[10] = { 2, 3, 5 , 7, 11, 13, 17, 19, 23, 29};
void *routine(void *arg)
{
    int value = *(int *)arg;
    // printf("%d\n", value);
    printf("%d\n", primes[value]);
    free(arg);
}
int main()
{
    pthread_t t[10];

    int i = 0;

    // pthread_mutex_init(&mutex, NULL);

    for (i = 0; i < 10; i++)
    {
        int *a =  malloc(sizeof(int));
        *a = i;
        // pthread_create(t + i, NULL, &routine, &primes[i]);
        pthread_create(t + i, NULL, &routine, a);
    }
    
    i = 0;
    for (i = 0; i < 10; i++)
    {
        pthread_join(t[i], NULL);
    }

    // pthread_mutex_destroy(&mutex);
}
