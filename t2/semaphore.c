#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
# include <semaphore.h>

sem_t   semaphore;
void    *routine(void   *arg)
{
    sem_wait(&semaphore);
    sleep(1);
    printf("hello thread : %d\n", *(int *)arg);
    sem_post(&semaphore);
    free(arg);
}


int main()
{
    pthread_t t[4];
    sem_init(&semaphore, 0, 1);

    for (int i = 0; i < 4; i++)
    {
        int *a = malloc(sizeof(int));
        *a = i;
        pthread_create(t + i, NULL, &routine, a);
    }
    
    for (int i = 0; i < 4; i++)
    {
        pthread_join(t[i], NULL);
    }

    sem_destroy(&semaphore);


}