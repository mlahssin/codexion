# include "codexion.h"

int init_sim(t_shared   *shared, t_coder    **coders, pthread_t **coders_th)
{
    if (params_allocation(coders, &shared->dongles, coders_th, shared) == 1)
        return -1;
    if (params_initialisation(shared, *coders, shared->dongles) == -1)
    {
        free_allocation(shared, *coders, *coders_th);
        return -1;
    }
    return 0;
}

int run_simulatoin(t_shared *shared, t_coder    *coders, pthread_t  *coders_th, pthread_t    *monitor_th)
{
    if (create_monitor_thread(monitor_th, shared, coders, coders_th) == -1)
        return -1;
    if (create_coder_threads(coders_th, shared, coders) == -1)
        return -1;
    if (join_coders_threads(coders_th, shared, coders) == -1)
        return -1;
    if (join_monitor_thread(monitor_th, shared, coders, coders_th) == -1)
        return -1;
    clean_all(shared, coders, coders_th);
    return 0;
}

int main(int ac, char *av[])
{
    t_shared   shared;
    pthread_t    monitor;
    pthread_t *coders_th;
    t_coder *coders;
    
    if (!parse(ac, av, &shared.params))
    {
        write(2, "Error\n", 6);
        return (1);
    }
    coders_th = NULL;
    coders = NULL;
    if (init_sim(&shared, &coders, &coders_th) == -1)
        return -1;
    if (run_simulatoin(&shared, coders, coders_th, &monitor) == -1)
        return -1;
    return 0;
}