#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
# include <semaphore.h>

sem_t   semaphore;
void    *routine(void   *arg)
{
    printf("%d waiting for the login queue\n", *(int *)arg);
    sem_wait(&semaphore);
    printf("%d logged in the loging queue\n", *(int *)arg);
    sleep(4);
    printf("%d logged out \n", *(int *)arg);
    sem_post(&semaphore);
    free(arg);
}


int main()
{
    pthread_t t[20];
    sem_init(&semaphore, 0, 12);

    for (int i = 0; i < 20; i++)
    {
        int *a = malloc(sizeof(int));
        *a = i;
        pthread_create(t + i, NULL, &routine, a);
    }
    
    for (int i = 0; i < 20; i++)
    {
        pthread_join(t[i], NULL);
    }

    sem_destroy(&semaphore);


}