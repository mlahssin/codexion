# include <pthread.h>
# include <stdio.h>
# include <stdlib.h>
# include <time.h>
# include <unistd.h>


typedef struct
{
    int items;
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
}   t_mu_co;



void    *add(void *arg)
{
    t_mu_co *check = (t_mu_co *)arg;
    for (int i = 0; i < 10; i++)
    {
        pthread_mutex_lock(&check->mutex);
        check->items++;
        printf("item now adding : %d\n", check->items);
        pthread_cond_signal(&check->cond);
        pthread_mutex_unlock(&check->mutex);
        usleep(1000);

    }
    return NULL;
}

void    *mince(void *arg)
{
    t_mu_co *check = (t_mu_co *)arg;

    for (int i = 0; i < 5; i++)
    {
        pthread_mutex_lock(&check->mutex);
        while (check->items < 1)
        {
            printf("not enough ressources!\n");
            pthread_cond_wait(&check->cond, &check->mutex);
        }


        check->items--;
        printf("item now mincing : %d\n", check->items);
        pthread_mutex_unlock(&check->mutex);

    }
    return NULL;
}

int main()
{
    pthread_t pro, cons;
    pthread_mutex_t mtx;
    pthread_cond_t c_d;
    t_mu_co check = {0, mtx, c_d};


    pthread_mutex_init(&check.mutex, NULL);
    pthread_cond_init(&check.cond, NULL);


    if (pthread_create(&pro, NULL, add, &check) != 0)
        return 1;
    if (pthread_create(&cons, NULL, mince, &check) != 0)
        return 2; 
    if (pthread_join(pro, NULL) != 0)
        return 3;
    if (pthread_join(cons, NULL) != 0)
        return 4;
    printf("items now : %d\n", check.items);

    pthread_mutex_destroy(&check.mutex);
    pthread_cond_destroy(&check.cond);
}