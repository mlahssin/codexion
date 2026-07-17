# include "codexion.h"

int    params_allocation(t_coder   **coders, t_dongle   **dongles, pthread_t **coders_ths, t_shared   *shared)
{
    *coders_ths = malloc(sizeof(pthread_t) * shared->params.num_coders);
    *coders = malloc(sizeof(t_coder) * shared->params.num_coders);
    *dongles = malloc(sizeof(t_dongle) * shared->params.num_coders);
    if (!*dongles || !*coders || !*coders_ths)
    {
        free(*dongles);
        free(*coders);
        free(*coders_ths);
        return 1;
    }
    return 0;
}

int    create_coder_threads(pthread_t  *coders_th, t_shared    *shared, t_coder    *coders)
{
    int i = 0;
    while (i < shared->params.num_coders)
    {
        if (pthread_create(coders_th + i, NULL, routine, (void *)&coders[i]) != 0)
        {
            clean_all(shared, coders, coders_th);
            return -1;
        }
        i++;
    }
    return 0;
}

int join_coders_threads(pthread_t  *coders_th, t_shared    *shared, t_coder    *coders)
{
    int i = 0;
    while (i < shared->params.num_coders)
    {
        if (pthread_join(coders_th[i], NULL) != 0)
        {
            clean_all(shared, coders, coders_th);
            return -1;
        }
        i++;
    }
    return 0;
}

int create_monitor_thread(pthread_t *monitor, t_shared  *shared, t_coder    *coders, pthread_t  *coders_th)
{
    if (pthread_create(monitor, NULL, monitoring, (void *)shared) != 0)
    {
        clean_all(shared, coders, coders_th);
        return -1;
    }
    return 0;
}

int join_monitor_thread(pthread_t *monitor, t_shared  *shared, t_coder    *coders, pthread_t  *coders_th)
{
    if(pthread_join(*monitor, NULL))
    {
        clean_all(shared, coders, coders_th);
        return -1;  
    }
    return 0;
}