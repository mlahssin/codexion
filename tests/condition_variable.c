# include <stdio.h>
# include <time.h>
#include <stdlib.h>
#include <pthread.h>
# include <unistd.h>

int fuel = 0;
pthread_mutex_t mtx_fu;
pthread_cond_t  condFuel;

void*   fuel_filling(void *arg)
{
    for (int i = 0; i < 5; i++)
    {
        pthread_mutex_lock(&mtx_fu);
        fuel += 15;
        printf("fule now is : %d\n", fuel);
        pthread_mutex_unlock(&mtx_fu);
        // pthread_cond_signal(&condFuel);
        pthread_cond_broadcast(&condFuel);
        sleep(1);
    }
    return NULL;
}


void*   car(void *arg)
{
    pthread_mutex_lock(&mtx_fu);
    while (fuel < 50)
    {
        printf("waiting\n");
        pthread_cond_wait(&condFuel, &mtx_fu);
        //equivalant to:
        //pthread_mutex_unlock(&mtx_fu);
        //pthread_cond_signal(&condFuel);
        //pthread_mutex_lock(&mtx_fu);
    }
    fuel -= 50;
    printf("left fuel %d\n", fuel);
    pthread_mutex_unlock(&mtx_fu);

    return NULL;
}

//pthread_cond_wait : unlock mutex (so other threads can change the shared data state)
// + sleep the thread (so it wait until the signal or the brodcast wake it up)
//then it rellock the mutex
//pthread_cond_signal : wake up one waiting thread
// Wakes up at least one thread sleeping on cond
// If no threads are waiting, the signal is lost (not queued)
// The woken thread will then re-acquire the mutex and re-check the condition

//pthread_cond_brodcast : wake up all threads waiting on cond

int main()
{
    pthread_t th[5];
    pthread_mutex_init(&mtx_fu, NULL);
    pthread_cond_init(&condFuel, NULL);
    for (int i = 0; i < 5; i++)
    {
        if (i == 4)
        {
            pthread_create(th + i, NULL, fuel_filling, NULL);
        }
        else
        {
            pthread_create(th + i, NULL, car, NULL);
        }    
    }

    for (int i = 0; i < 2; i++)
    {
        pthread_join(th[i], NULL);
    }

    pthread_mutex_destroy(&mtx_fu);



    return 0;
}