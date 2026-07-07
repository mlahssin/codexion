#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
// pthread_cond_wait inside a while loop and not if ??
int fuel = 0;

pthread_mutex_t mutexFuel;
pthread_cond_t condFuel;


void *fuel_filling(void *arg)
{
    for (int i = 0; i < 5; i++)
    {
        pthread_mutex_lock(&mutexFuel);
        fuel += 15;
        printf("Filled fuel ... %d\n", fuel);
        pthread_mutex_unlock(&mutexFuel);
        pthread_cond_signal(&condFuel);
        sleep(1);
    }
}

void *car(void *arg)
{
        pthread_mutex_lock(&mutexFuel);
        while(fuel < 40)
        {
            printf("no fuel waiting\n");
            pthread_cond_wait(&condFuel, &mutexFuel);
        }
        fuel -= 40;
        printf("fuel left :%d\n", fuel);
        pthread_mutex_unlock(&mutexFuel);
}

// pthread_cond_wait
// it is equivalent to :  
// pthread_mutex_unlock(&mutexFuel);
// waitfor singal on condFuel
// pthread_mutex_lock(&mutexFuel);




// pthread_cond_broadcastfa
// pthread_cond_signal

int main()
{
    pthread_t t[2];
    pthread_mutex_init(&mutexFuel, NULL);
    pthread_cond_init(&condFuel, NULL);
    for (int i = 0; i < 2; i++)
    {
        if (i == 0)
            pthread_create(t + i, NULL, &fuel_filling, NULL);
        else
            pthread_create(t + i, NULL, &car, NULL);
    }
    
    for (int i = 0; i < 2; i++)
    {
        pthread_join(t[i], NULL);
    }

    pthread_mutex_destroy(&mutexFuel);
    pthread_cond_destroy(&condFuel);
}