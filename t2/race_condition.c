#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>



int mails = 0;


// read mails
// increment it 
// write it back to memory
void *routine()
{
    for (int i = 0; i < 1000; i++)
        mails++;
}
// why the race condition doesn't happen in a small number of iterations
// the iterations of the first thread will finish before the second one created
// but it may happen in small numbers either if it's probalbily to happen is low
int main()
{
        pthread_t t1, t2;

        pthread_create(&t1, NULL, &routine, NULL);

    pthread_create(&t2, NULL, &routine, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    printf("N of mails : %d\n", mails);
    
}