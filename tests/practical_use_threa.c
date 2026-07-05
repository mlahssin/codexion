# include <stdio.h>
# include <time.h>
#include <stdlib.h>
#include <pthread.h>


int t[10] = {1, 9, 5, 3, 2, 4, 6, 11, 20, 90};

void* suming(void* arg)
{
    int *elt = (int *)arg;
    int sum = 0;
    for (int i = 0; i < 5; i++)
    {
        sum += t[*elt + i];
    }
    *elt = sum;
    return elt;
}


int main()
{
    pthread_t th[2];
    void *ret;
    int global_sum = 0;
    int i = 0;
    for (i = 0; i < 2; i++)
    {
        int *index = malloc(sizeof(int));
        *index = i * 5;
        pthread_create(&th[i], NULL, suming, index);
        // pthread_join(th[i], &ret);

    }
    i = 0;
    for (i = 0; i < 2; i++)
    {
        pthread_join(th[i], &ret);
        global_sum += *(int *)ret;
        free(ret);
    }

    printf("%d\n", global_sum);
}