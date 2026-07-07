#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
# include <string.h>

pthread_barrier_t roll_barrier;
int diceValue[8];
int status[8] = {0};
void *rolldice(void *arg)
{
    int index = *(int *)arg;
    diceValue[index] = rand() % 6 + 1;
    pthread_barrier_wait(&roll_barrier);
    if (status[index] == 1)
        printf("%d rolled %d I won\n", index, diceValue[index]);
    else    
        printf("%d rolled %d I losts\n", index, diceValue[index]);
    free(arg);
}

int main()
{
    srand(time(NULL));

    pthread_t t[8];
    pthread_barrier_init(&roll_barrier, NULL, 9);
    for (int i = 0; i < 8; i++)
    {
        int *a = malloc(sizeof(int));
        *a = i;
        pthread_create(t + i, NULL, &rolldice, a);
    }
    pthread_barrier_wait(&roll_barrier);
    
    int max = 0;
    for (int i = 0; i < 8; i++)
    {
        if (diceValue[i] > max)
            max = diceValue[i];
    }
    for (int i = 0; i < 8; i++)
    {
        if (diceValue[i] == max)
            status[i] = 1;
        else
            status[i] = 0;
    }

    for (int i = 0; i < 8; i++)
    {
        pthread_join(t[i], NULL);
    }
    pthread_barrier_destroy(&roll_barrier);
}