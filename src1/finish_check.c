# include "codexion.h"

bool    all_coders_done_compiling(t_coder   *coders, int num_coders)
{
    int i;

    i = 0;
        while(i < num_coders)
        {
            pthread_mutex_lock(&coders[i].compile_count_mutex);
            if (coders[i].compile_count < coders[i].shared->params.num_compiles)
            {
                pthread_mutex_unlock(&coders[i].compile_count_mutex);
                return false;
            }
             pthread_mutex_unlock(&coders[i].compile_count_mutex);
            i++;
        }
    return true;
}


int    is_burnout(t_coder  *coders, int num_coders)
{
    int i = 0;
    
    while (i < num_coders)
    {
        pthread_mutex_lock(&coders[i].compile_count_mutex);
        if(coders[i].last_compile_start + coders[i].shared->params.time_to_burnout <= elapsed_time(coders[i].shared->start))
        {
            pthread_mutex_unlock(&coders[i].compile_count_mutex);
            return coders[i].id;
        }
        pthread_mutex_unlock(&coders[i].compile_count_mutex);
        i++;
    }
    return -1;
}

void    set_stop(t_shared  *shared)
{
    pthread_mutex_lock(&shared->stop_mutex);
    shared->stop = 1;
    pthread_mutex_unlock(&shared->stop_mutex);
    for (int i = 0; i < shared->params.num_coders; i++)
    {
        pthread_mutex_lock(&shared->dongles[i].dongle_mutex);
        pthread_cond_broadcast(&shared->dongles[i].dongle_wait);
        pthread_mutex_unlock(&shared->dongles[i].dongle_mutex);
    }
}

int    get_stop(t_shared  *shared)
{
    int value;
    pthread_mutex_lock(&shared->stop_mutex);
    value = shared->stop;
    pthread_mutex_unlock(&shared->stop_mutex);
    return value;
}

void    *monitoring(void *arg)
{
    t_shared   *shared;

    shared = (t_shared *)arg;
    while(get_stop(shared) == 0)
    {
        int burnout_coder = is_burnout(shared->coders, shared->params.num_coders);
        if(burnout_coder != -1)
        {
            set_stop(shared);
            print_lock(burnout_coder, 4,shared);
            return NULL;
        }

        if(all_coders_done_compiling(shared->coders, shared->params.num_coders))
        {
            set_stop(shared);
            return NULL;
        }
        smart_sleep(1, shared); 
    }
    return NULL;
}