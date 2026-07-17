# include "codexion.h"

void    release_dongle(t_dongle *dongle, t_coder  *coder)
{
    pthread_mutex_lock(&dongle->dongle_mutex);
    if (get_stop(coder->shared) != 0)
    {
        pthread_mutex_unlock(&dongle->dongle_mutex);
        return;
    }
    dongle->used = -1;
    dongle->released_at = elapsed_time(coder->shared->start) + coder->shared->params.cooldown;
    pthread_mutex_lock(&coder->compile_count_mutex);
    coder->num_dongles_held--;
    pthread_mutex_unlock(&coder->compile_count_mutex);
    
    pthread_cond_broadcast(&dongle->dongle_wait);
    pthread_mutex_unlock(&dongle->dongle_mutex);
}

void    push_coder_to_dongle_heap(t_dongle *dongle, t_coder    *coder)
{
    t_waiter    w;
    w.coder_id = coder->id;


    if (coder->shared->params.scheduler_type == 0)
        w.prioroty = now_ms();
    else
    {
        pthread_mutex_lock(&coder->compile_count_mutex);
        w.prioroty = coder->last_compile_start + coder->shared->params.time_to_burnout;
        pthread_mutex_unlock(&coder->compile_count_mutex);
    }
    pthread_mutex_lock(&dongle->dongle_mutex);
    if (get_stop(coder->shared)!= 0)
    {
        pthread_mutex_unlock(&dongle->dongle_mutex);
        return;
    }
    push(dongle, w);

    pthread_mutex_unlock(&dongle->dongle_mutex);
}

void mark_dongle_taken(t_dongle *dongle, t_coder *coder)
{
    dongle->used = coder->id;
    coder->num_dongles_held++;
    pop(dongle);
    pthread_cond_broadcast(&dongle->dongle_wait);
    print_dongles_taken(coder);
}

void    take_dongle(t_dongle *dongle, t_coder    *coder)
{
    struct timespec ts;
    long long time_left;

    pthread_mutex_lock(&dongle->dongle_mutex);
    while(get_stop(coder->shared) == 0)
    {
        if(can_coder_take_dongle(dongle, coder))
        {  
            mark_dongle_taken(dongle, coder);
            pthread_mutex_unlock(&dongle->dongle_mutex);
            return;
        }
        else if(is_only_cooldown_left(dongle, coder))
        {
            time_left = dongle->released_at - elapsed_time(coder->shared->start);
            build_timeout_ts(&ts, time_left);
            pthread_cond_timedwait(&dongle->dongle_wait, &dongle->dongle_mutex, &ts);
        }
        else
            pthread_cond_wait(&dongle->dongle_wait, &dongle->dongle_mutex);
    }
    pthread_mutex_unlock(&dongle->dongle_mutex);
}


void    aquire_dongles(t_coder    *coder)
{
    push_coder_to_dongle_heap(&coder->shared->dongles[coder->dongle_index_1], coder);
    take_dongle(&coder->shared->dongles[coder->dongle_index_1], coder);
    push_coder_to_dongle_heap(&coder->shared->dongles[coder->dongle_index_2], coder);
    take_dongle(&coder->shared->dongles[coder->dongle_index_2], coder);    

}