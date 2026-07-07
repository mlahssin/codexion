#include <pthread.h>
#include <stdio.h>
#include <string.h>

char message[20] = "Hello";
pthread_mutex_t mutex;
void *thread1(void *arg)
{
    pthread_mutex_lock(&mutex);

    strcpy(message, "Apple");
    printf("Thread 1: %s\n", message);
    pthread_mutex_unlock(&mutex);

    return NULL;
}

void *thread2(void *arg)
{
    pthread_mutex_lock(&mutex);

    strcpy(message, "orange");
    printf("Thread 2: %s\n", message);
    pthread_mutex_unlock(&mutex);

    return NULL;
}

int main()
{
    pthread_t t1, t2;
    pthread_mutex_init(&mutex, NULL);
    pthread_create(&t1, NULL, thread1, NULL);
    pthread_create(&t2, NULL, thread2, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("Final message: %s\n", message);
    pthread_mutex_destroy(&mutex);
}