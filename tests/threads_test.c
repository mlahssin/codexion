#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

void    *worker(void *arg)
{
    int id;

    id = *(int *)arg;
    printf("Worker %d starting\n", id);
    usleep(1000000);    // simulate 1 second of work
    printf("Worker %d done\n", id);
    return (NULL);
}

int main(void)
{
    pthread_t   threads[4];
    int         ids[4];
    int         i;

    i = 0;
    while (i < 4)
    {
        ids[i] = i + 1;
        pthread_create(&threads[i], NULL, worker, &ids[i]);
        i++;
    }
    i = 0;
    while (i < 4)
    {
        pthread_join(threads[i], NULL);
        i++;
    }
    printf("All workers done\n");
    return (0);
}