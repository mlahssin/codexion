// pthread_mutex_lock()
// This function blocks until the mutex becomes available


// pthread_mutex_trylock()

// This function does not wait.

// Returns 0 if it successfully acquires the mutex.
// Returns EBUSY if another thread already owns the mutex.


#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
# include <errno.h>


pthread_mutex_t mutex;
void *routine()
{

    if (pthread_mutex_trylock(&mutex) == 0)
    {
        printf("locked\n");
        sleep(1);
        pthread_mutex_unlock(&mutex);
    }
    else
        printf("didn't get locke\n");
    return NULL;
}

int main()
{
    pthread_t t[4];
    int i = 0;

    pthread_mutex_init(&mutex, NULL);

    for (i = 0; i < 4; i++)
    {
        pthread_create(t + i, NULL, &routine, NULL);
    }
    
    i = 0;
    for (i = 0; i < 4; i++)
    {
        pthread_join(t[i], NULL);
    }

    pthread_mutex_destroy(&mutex);
}