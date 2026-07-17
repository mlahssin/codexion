# include "codexion.h"

void    clean_all(t_shared  *shared, t_coder    *coders, pthread_t  *coders_threads)
{
    int i;

    i = 0;
    while(i < shared->params.num_coders)
    {
        pthread_mutex_destroy(&shared->coders[i].compile_count_mutex);
        i++;
    }
    i = 0;
    while(i < shared->params.num_coders)
    {
        pthread_mutex_destroy(&shared->dongles[i].dongle_mutex);
        pthread_cond_destroy(&shared->dongles[i].dongle_wait);
        i++;
    }
    pthread_mutex_destroy(&shared->print_mutex);
    pthread_mutex_destroy(&shared->stop_mutex);
    free_allocation(shared, coders, coders_threads);
}

void    free_allocation(t_shared  *shared, t_coder    *coders, pthread_t  *coders_threads)
{
    free(shared->dongles);
    free(coders);
    free(coders_threads); 
}