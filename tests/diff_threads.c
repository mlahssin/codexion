#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

void    *downloader(void *arg)
{
    (void)arg;
    printf("Downloading file...\n");
    usleep(3000000);
    printf("Download complete\n");
    return (NULL);
}

void    *processor(void *arg)
{
    (void)arg;
    printf("Processing data...\n");
    usleep(2000000);
    printf("Processing complete\n");
    return (NULL);
}

void    *monitor(void *arg)
{
    int i;

    (void)arg;
    i = 0;
    while (i < 5)
    {
        printf("System running... %d\n", i);
        usleep(1000000);
        i++;
    }
    return (NULL);
}

int main(void)
{
    pthread_t   t1;
    pthread_t   t2;
    pthread_t   t3;

    pthread_create(&t1, NULL, downloader, NULL);
    pthread_create(&t2, NULL, processor, NULL);
    pthread_create(&t3, NULL, monitor, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
    return (0);
}