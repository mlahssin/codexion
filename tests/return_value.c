# include <stdio.h>
# include <time.h>
#include <stdlib.h>
#include <pthread.h>

void*   roll_dice()
{
    int *res = malloc(sizeof(int));
    if (!res)
        return NULL;

    int value = (rand() % 6) + 1;
    printf("%d\n", value);
    *res = value;
    return (void *) res;
}


int main()
{
    int *res;
    srand(time(NULL));
    pthread_t th;
    if (pthread_create(&th, NULL, &roll_dice, NULL) != 0)
        return 1;
    if (pthread_join(th, (void **) &res) != 0)
        return 2;
    printf("Main res: %p\n", res);
    printf("result: %d\n", *res);
    free(res);
    return 0;
}