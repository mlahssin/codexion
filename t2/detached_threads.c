#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
# include <string.h>

void    *routine(void *arg)
{
    sleep(1);
    printf("finished execution \n");
}



int main()
{
    pthread_t t[4];
    int i = 0;


    for (i = 0; i < 4; i++)
    {
        pthread_create(t + i, NULL, &routine, NULL);
        pthread_detach(t[i]);
    }
    
    i = 0;
    // for (i = 0; i < 4; i++)
    // {
    //     if (pthread_join(t[i], NULL) != 0)
    //         perror("failed to join  thread");
    // }
    pthread_exit(0);


}