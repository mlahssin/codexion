#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>



int mails = 0;
pthread_mutex_t mutex;

// read mails
// increment it 
// write it back to memory
void *routine()
{
    for (int i = 0; i < 1000000; i++)
    {
        pthread_mutex_lock(&mutex);
        mails++;
        pthread_mutex_unlock(&mutex);
    }
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
    printf("value of x is : %d\n", mails);


}