# include "codexion.h"

void    set_stop(t_shared  *shared)
{
    pthread_mutex_lock(&shared->stop_mutex);
    shared->stop = 1;
    for (int i = 0; i < shared->params.num_coders; i++)
    {
        pthread_cond_broadcast(&shared->dongles[i].dongle_wait);
    }
    pthread_mutex_unlock(&shared->stop_mutex);
}

int    get_stop(t_shared  *shared)
{
    int value;
    pthread_mutex_lock(&shared->stop_mutex);
    value = shared->stop;
    pthread_mutex_unlock(&shared->stop_mutex);
    return value;
}

long long now_ms()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return  now.tv_sec * 1000LL + now.tv_usec / 1000LL;
}

long long elapsed_time(long long start)
{
    return now_ms() - start;
}

void    smart_sleep(int time, t_shared *shared)
{
    int start = elapsed_time(shared->start);
    while(get_stop(shared) == 0)
    {
        if(get_stop(shared) == 1)
            break;
        if ( elapsed_time(shared->start) >= start + time / 1000)//check this
            break;
        usleep(1000);
    }
}

bool is_dongle_available(t_dongle   *dongle)
{
    int value;
    value = 0;
    value = dongle->used;
    return value == -1;
}

bool    cooldown_finished(t_dongle   *dongle, int start)
{
    return elapsed_time(start) >= dongle->released_at;
}

void    print_lock(int  id, int flag, t_shared *shared)
{
    pthread_mutex_lock(&shared->print_mutex);
    if (flag == 0)
    {
        if (get_stop(shared) != 0)
        {
            pthread_mutex_unlock(&shared->print_mutex);
            return;
        }
        printf("%lld %d has taken a dongle\n", elapsed_time(shared->start), id + 1);
    }
        
    else if (flag == 1)
    {
        if (get_stop(shared) != 0)
        {
            pthread_mutex_unlock(&shared->print_mutex);
            return;
        }
        printf("%lld %d is compiling\n", elapsed_time(shared->start), id + 1);
    }
    else if (flag == 2)
    {
        if (get_stop(shared) != 0)
        {
            pthread_mutex_unlock(&shared->print_mutex);
            return;
        }
        printf("%lld %d is debugging\n", elapsed_time(shared->start), id + 1);
    }
    else if (flag == 3)
    {
        if (get_stop(shared) != 0)
        {
            pthread_mutex_unlock(&shared->print_mutex);
            return;
        }
        printf("%lld %d is refactoring\n", elapsed_time(shared->start), id + 1);
    }
    else
    {
        printf("%lld %d burned out\n", elapsed_time(shared->start), id + 1);
    }
    pthread_mutex_unlock(&shared->print_mutex);
}

bool    all_coders_done_compiling(t_coder   *coders, int num_coders)
{
    int i = 0;
        while(i < num_coders)
        {
            if (coders[i].compile_count < coders[i].shared->params.num_compiles)
                return false;
            i++;
        }
    return true;
}


int    is_burnout(t_coder  *coders, int num_coders)
{
    int i = 0;
    while (i < num_coders)
    {
        if(coders[i].last_compile_start + coders[i].shared->params.time_to_burnout <= elapsed_time(coders[i].shared->start))
            return coders[i].id;
        i++;
    }
    return -1;
}

void    free_dongle(t_dongle *dongle, t_shared  *shared)
{
    pthread_mutex_lock(&dongle->dongle_mutex);
    if (get_stop(shared) != 0)
    {
        pthread_mutex_unlock(&dongle->dongle_mutex);
        return;
    }
    dongle->used = -1;
    dongle->released_at = elapsed_time(shared->start) + shared->params.cooldown;
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
        w.prioroty = coder->last_compile_start + coder->shared->params.time_to_burnout;
    pthread_mutex_lock(&dongle->dongle_mutex);
    if (get_stop(coder->shared)!= 0)
    {
        pthread_mutex_unlock(&dongle->dongle_mutex);
        return;
    }
    push(dongle, w);

    pthread_mutex_unlock(&dongle->dongle_mutex);
}

bool    is_only_cooldown_left(t_dongle *dongle, t_coder    *coder)
{
    return(extract_min(dongle) == coder->id && is_dongle_available(dongle) && !cooldown_finished(dongle, coder->shared->start));
}

bool    can_coder_take_dongle(t_dongle *dongle, t_coder    *coder)
{
    return(is_dongle_available(dongle)  && extract_min(dongle) == coder->id && cooldown_finished(dongle, coder->shared->start));
}

void    build_timeout_ts(struct timespec *ts, long long ms_from_now)
{
    struct timeval now;
    long long sec_add;
    long long nsec_add;

    gettimeofday(&now, NULL);
    ts->tv_sec = now.tv_sec;
    ts->tv_nsec = now.tv_usec * 1000;

    sec_add = ms_from_now / 1000;
    nsec_add = (ms_from_now  % 1000) * 1000000;
    ts->tv_sec += sec_add;
    ts->tv_nsec += nsec_add;
    if (ts->tv_nsec >= 1000000000)
    {
        ts->tv_sec += 1;
        ts->tv_nsec -= 1000000000;
    }
}

void    print_dongles_taken(t_coder    *coder)
{
    if (coder->num_dongles_held == 2)
    {
        print_lock(coder->id, 0, coder->shared);
        print_lock(coder->id, 0, coder->shared);
    }
}
void    take_dongle(t_dongle *dongle, t_coder    *coder)
{
    pthread_mutex_lock(&dongle->dongle_mutex);
    struct timespec ts;
    long long time_left = dongle->released_at - elapsed_time(coder->shared->start);

    build_timeout_ts(&ts, time_left);
    while(get_stop(coder->shared) == 0)
    {
        if(can_coder_take_dongle(dongle, coder))
        {  
            dongle->used = coder->id;
            coder->num_dongles_held++;
            pop(dongle);
            pthread_cond_broadcast(&dongle->dongle_wait);
            print_dongles_taken(coder);
            pthread_mutex_unlock(&dongle->dongle_mutex);
            return;
        }
        else if(is_only_cooldown_left(dongle, coder))
            pthread_cond_timedwait(&dongle->dongle_wait, &dongle->dongle_mutex, &ts);
        else
            pthread_cond_wait(&dongle->dongle_wait, &dongle->dongle_mutex);
    }
    pthread_mutex_unlock(&dongle->dongle_mutex);
}


void    require_dongles(t_coder    *coder)
{
    push_coder_to_dongle_heap(&coder->shared->dongles[coder->dongle_index_1], coder);
    take_dongle(&coder->shared->dongles[coder->dongle_index_1], coder);
    push_coder_to_dongle_heap(&coder->shared->dongles[coder->dongle_index_2], coder);
    take_dongle(&coder->shared->dongles[coder->dongle_index_2], coder);    

}

void    compile_phase(t_coder *coder)
{
    print_lock(coder->id, 1,coder->shared);
    coder->last_compile_start = elapsed_time(coder->shared->start);
    smart_sleep(coder->shared->params.time_to_compile * 1000, coder->shared);
    coder->compile_count++;
}

void    debug_phase(t_coder *coder)
{
    print_lock(coder->id, 2,coder->shared);
    smart_sleep(coder->shared->params.time_to_debug * 1000, coder->shared);  
}

void    refactor_phase(t_coder  *coder)
{
    print_lock(coder->id, 3,coder->shared);
    smart_sleep(coder->shared->params.time_to_refactor * 1000, coder->shared);
}
void    *routine(void    *arg)
{                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  
    t_coder *coder = (t_coder   *)arg;

    while(get_stop(coder->shared) == 0)
    {
        require_dongles(coder);

        // push_coder_to_dongle_heap(&coder->shared->dongles[coder->dongle_index_1], coder);
        // take_dongle(&coder->shared->dongles[coder->dongle_index_1], coder);
        // push_coder_to_dongle_heap(&coder->shared->dongles[coder->dongle_index_2], coder);
        // take_dongle(&coder->shared->dongles[coder->dongle_index_2], coder);

        if(get_stop(coder->shared) == 1)
            return NULL;

        // print_lock(coder->id, 1,coder->shared);
        // coder->last_compile_start = elapsed_time(coder->shared->start);
        // smart_sleep(coder->shared->params.time_to_compile * 1000, coder->shared);
        // coder->compile_count++;
        compile_phase(coder);
        free_dongle(&coder->shared->dongles[coder->dongle_index_1], coder->shared);
        free_dongle(&coder->shared->dongles[coder->dongle_index_2], coder->shared);
        
        if(get_stop(coder->shared) == 1)
        {
            return NULL;
        }
        debug_phase(coder);
        // print_lock(coder->id, 2,coder->shared);
        // smart_sleep(coder->shared->params.time_to_debug * 1000, coder->shared);
        if(get_stop(coder->shared) == 1)
        {
            return NULL;
        }
        refactor_phase(coder);
        // print_lock(coder->id, 3,coder->shared);
        // smart_sleep(coder->shared->params.time_to_refactor * 1000, coder->shared);
    }
    return NULL;
}


void    *monitoring(void *arg)
{
    t_shared   *shared = (t_shared *)arg;


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
        smart_sleep(1000, shared); 
    }
    return NULL;
}

int main(int ac, char *av[])
{
    t_shared   shared;
    if (!parse(ac, av, &shared.params))
    {
        write(2, "Error\n", 6);
        return (1);
    }

    pthread_t    monitor;
    pthread_t *coder_ths = malloc(sizeof(pthread_t) * shared.params.num_coders);

    t_coder *coders = malloc(sizeof(t_coder ) * shared.params.num_coders);
    t_dongle   *dongles = malloc(sizeof(t_dongle) * shared.params.num_coders);
    shared.dongles = dongles;

    dongle_init(&shared);

    coders_init(coders, &shared);
    sim_init(&shared, coders, dongles);
    pthread_create(&monitor, NULL, monitoring, (void *)&shared);
    for (int i = 0; i < shared.params.num_coders; i++)
    {
        pthread_create(coder_ths + i, NULL, routine, (void *)&coders[i]);
    }

    for (int i = 0; i < shared.params.num_coders; i++)
    {
        pthread_join(coder_ths[i], NULL);
    }
    pthread_join(monitor, NULL);

    for (int i = 0; i < shared.params.num_coders; i++)
    {
        pthread_mutex_destroy(&shared.dongles[i].dongle_mutex);
        pthread_cond_destroy(&shared.dongles[i].dongle_wait);

    }
    pthread_mutex_destroy(&shared.print_mutex);
    pthread_mutex_destroy(&shared.stop_mutex);
    free(coder_ths);
    free(coders);
    free(dongles);
}