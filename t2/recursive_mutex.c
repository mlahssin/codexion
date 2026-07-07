#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
# include <string.h>

pthread_mutex_t mutexFuel;
int fuel = 10;
void *routine(void *arg)
{
    pthread_mutex_lock(&mutexFuel);
    pthread_mutex_lock(&mutexFuel);
    fuel += 20;
    printf("fuel now is : %d\n",fuel);
    pthread_mutex_unlock(&mutexFuel);
    pthread_mutex_unlock(&mutexFuel);

}

int main()
{
    pthread_t t[4];
    int i = 0;
    pthread_mutexattr_t recursiveMutexatt;
    pthread_mutexattr_settype(&recursiveMutexatt, PTHREAD_MUTEX_RECURSIVE);

    pthread_mutex_init(&mutexFuel, &recursiveMutexatt);

    for (i = 0; i < 4; i++)
    {
        pthread_create(t + i, NULL, &routine, NULL);
    }
    
    i = 0;
    for (i = 0; i < 4; i++)
    {
        pthread_join(t[i], NULL);
    }
    pthread_mutexattr_destroy(&recursiveMutexatt);
    pthread_mutex_destroy(&mutexFuel);
}