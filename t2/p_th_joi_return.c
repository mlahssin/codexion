#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
# include <time.h>


void *roll_dice()
{
    int value = (rand() % 6) + 1;
    int *result = malloc(sizeof(int));
    *result = value;
    printf("the address is : %p\n", result);
    return (void *)result;   
}


int main()
{
    srand(time(NULL));
    pthread_t t1;
    int *res;
    pthread_create(&t1, NULL, &roll_dice, NULL);
    pthread_join(t1, (void **)&res);
    printf("Main re : %p\n", res);
    printf("the result is  %d\n", *res);
}