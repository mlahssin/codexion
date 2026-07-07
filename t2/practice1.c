#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int primes[10] = { 1, 2, 3 , 4, 5, 6, 7, 8, 9, 10};

void *routine(void *arg)
{
    int sum = 0;
    int index = *(int *)arg;
    // int *res = malloc(sizeof(int));
    for (int i = 0 ; i < 5; i++)
    {
        sum += primes[i + index];
    }
    *(int *)arg = sum;
    // return (void *)res;
    return arg;
}


int main()
{
    pthread_t t[2];

    int i = 0;


    // pthread_mutex_init(&mutex, NULL);

    for (i = 0; i < 2; i++)
    {
        int *a = malloc(sizeof(int));
        *a = i * 5;
        pthread_create(t + i, NULL, &routine, a);
    }
    int somme = 0;
    i = 0;
    for (i = 0; i < 2; i++)
    {
        int *result;
        pthread_join(t[i], (void **)&result);
        somme += *result;
        free(result);
    }
    printf("la somme est : %d", somme);
}
