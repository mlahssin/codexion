#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
# include <semaphore.h>

sem_t   semaphore;
void    *routine(void   *arg)
{
    int index = *(int*)arg;
    int se_val;
    sem_wait(&semaphore);
    sem_getvalue(&semaphore, &se_val);
    printf("%d current semaphore value is%d\n", index, se_val);
    sem_post(&semaphore);
    free(arg);
}


int main()
{
    pthread_t t[4];
    sem_init(&semaphore, 0, 4);

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