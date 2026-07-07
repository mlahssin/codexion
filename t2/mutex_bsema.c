#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
# include <semaphore.h>

sem_t   semaphore;
int *fuel;
pthread_mutex_t mutex;

void    *routine1(void   *arg)
{
    while(1)
    {
        // sem_wait(&semaphore);
        pthread_mutex_lock(&mutex);
        *fuel += 1;
        printf("cuurent value is %d\n", *fuel);
        // sem_post(&semaphore);
    }
    
}

void    *routine2(void *arg)
{
    while(1)
    {
        pthread_mutex_unlock(&mutex);

        // sem_post(&semaphore);
        usleep(500000);
    }
}


int main()
{
    pthread_t t[20];
    pthread_mutex_init(&mutex, NULL);
    // sem_init(&semaphore, 0, 1);
    fuel = malloc(sizeof(int));
    *fuel = 0;
    for (int i = 0; i < 2; i++)
    {
        if (i % 2 == 0)
            pthread_create(t + i, NULL, &routine1, NULL);
        else
            pthread_create(t + i, NULL, &routine2, NULL);

    }
    
    for (int i = 0; i < 5; i++)
    {
        pthread_join(t[i], NULL);
    }

    // sem_destroy(&semaphore);
    pthread_mutex_destroy(&mutex);

}