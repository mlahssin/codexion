#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


void    *routine()
{
    printf("start thread\n");
    sleep(1);
    printf("ending thread\n");
    // return NULL;
}


int main()
{
    pthread_t t1, t2;

    if (pthread_create(&t1, NULL, &routine, NULL) != 0)
        return 1;
    // return int 0 if thread created successufully non zero if not
    // first one : a pointer to pthread variale
    // thread attribttes
    // pointer to the function that the thread will execute
    // the arg passed to the thread function

    pthread_create(&t2, NULL, &routine, NULL);
    pthread_join(t1, NULL);
    // first : thread u want to wait for 
    // second where u want to store the thread return  if don't care NULL
    pthread_join(t2, NULL);
}