#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
# include <semaphore.h>

sem_t   semaphore;
int *fuel;
void    *routine(void   *arg)
{
    for (int i = 0; i < 100000; i++)
    {
        sem_wait(&semaphore);
        *fuel += 1;
        printf("cuurent value is %d\n", *fuel);
        sem_post(&semaphore);
    }
    
}


int main()
{
    pthread_t t[20];

    sem_init(&semaphore, 0, 1);
    fuel = malloc(sizeof(int));
    *fuel = 0;
    for (int i = 0; i < 5; i++)
    {
        int *a = malloc(sizeof(int));
        *a = i;
        pthread_create(t + i, NULL, &routine, a);
    }
    
    for (int i = 0; i < 5; i++)
    {
        pthread_join(t[i], NULL);
    }

    sem_destroy(&semaphore);


}