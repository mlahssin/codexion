#include <stdio.h>
#include <pthread.h>
// this the race condition (the problem) this what happen
    // multiple threads access the same data
    // at least one thread modifies it
    // and access is not synchronized
// int counter = 0;    // SHARED between all threads

// void    *increment(void *arg)
// {
//     int i;

//     i = 0;
//     while (i < 1000000)
//     {
//         counter++;    // looks simple — it's not
//         i++;
//     }
//     return (NULL);
// }

// int main(void)
// {
//     pthread_t   t1;
//     pthread_t   t2;

//     pthread_create(&t1, NULL, increment, NULL);
//     pthread_create(&t2, NULL, increment, NULL);
//     pthread_join(t1, NULL);
//     pthread_join(t2, NULL);
//     printf("Counter: %d\n", counter);
//     return (0);
// }



// this the soltion for that problem using mutex


int counter = 0;    // SHARED between all threads
pthread_mutex_t mutex;

void    *increment(void *arg)
{
    int i;

    i = 0;
    while (i < 1000000)
    {
        pthread_mutex_lock(&mutex);
        counter++;    // looks simple — it's not
        i++;
        pthread_mutex_unlock(&mutex);
    }
    
    return (NULL);
}

int main(void)
{
    pthread_t   t1;
    pthread_t   t2;
    pthread_mutex_init(&mutex, NULL);

    pthread_create(&t1, NULL, increment, NULL);
    pthread_create(&t2, NULL, increment, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    printf("Counter: %d\n", counter);
    return (0);
}