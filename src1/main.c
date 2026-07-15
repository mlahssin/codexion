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
        if ( elapsed_time(shared->start) >= start + time)//check this
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

bool    cooldown_finished(t_dongle   *dongle, long long start)
{
    // printf("now : %lld cooldown finished at : %lld\n",elapsed_time(start), dongle->released_at);
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

void    release_dongle(t_dongle *dongle, t_shared  *shared)
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

bool    is_only_cooldown_left(t_dongle *dongle, t_coder    *coder)
{
    return(extract_min(dongle) == coder->id && is_dongle_available(dongle) && !cooldown_finished(dongle, coder->shared->start));
}

bool    can_coder_take_dongle(t_dongle *dongle, t_coder    *coder)
{
    return(is_dongle_available(dongle) && extract_min(dongle) == coder->id && cooldown_finished(dongle, coder->shared->start));
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
    struct timespec ts;
    long long time_left;

    pthread_mutex_lock(&dongle->dongle_mutex);
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

void    compile_phase(t_coder *coder)
{
    print_lock(coder->id, 1,coder->shared);
    pthread_mutex_lock(&coder->compile_count_mutex);
    coder->last_compile_start = elapsed_time(coder->shared->start);
    pthread_mutex_unlock(&coder->compile_count_mutex);
    smart_sleep(coder->shared->params.time_to_compile, coder->shared);
    pthread_mutex_lock(&coder->compile_count_mutex);
    coder->compile_count++;
    pthread_mutex_unlock(&coder->compile_count_mutex);
}

void    debug_phase(t_coder *coder)
{
    print_lock(coder->id, 2,coder->shared);
    smart_sleep(coder->shared->params.time_to_debug, coder->shared);  
}

void    refactor_phase(t_coder  *coder)
{
    print_lock(coder->id, 3,coder->shared);
    smart_sleep(coder->shared->params.time_to_refactor, coder->shared);
}

void    release_both_dongles(t_coder *coder)
{
    release_dongle(&coder->shared->dongles[coder->dongle_index_1], coder->shared);
    release_dongle(&coder->shared->dongles[coder->dongle_index_2], coder->shared);
}

void    *routine(void    *arg)
{                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  
    t_coder *coder = (t_coder   *)arg;

    while(get_stop(coder->shared) == 0)
    {
        aquire_dongles(coder);

        if(get_stop(coder->shared) == 1)
            return NULL;

        compile_phase(coder);
        
        release_both_dongles(coder);
        if(get_stop(coder->shared) == 1)
            return NULL;

        debug_phase(coder);

        if(get_stop(coder->shared) == 1)
            return NULL;

        refactor_phase(coder);
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
void    params_allocation(t_coder   *coders, t_dongle   *dongles, pthread_t *coders_ths, t_shared   shared)
{
    coders_ths = malloc(sizeof(pthread_t) * shared.params.num_coders);
    coders = malloc(sizeof(pthread_t) * shared.params.num_coders);
    dongles = malloc(sizeof(t_dongle) * shared.params.num_coders);
}

int main(int ac, char *av[])
{
    t_shared   shared;
    // pthread_t    monitor;
    // pthread_t *coder_ths;
    // t_coder *coders;
    // t_dongle   *dongles;
    
    if (!parse(ac, av, &shared.params))
    {
        write(2, "Error\n", 6);
        return (1);
    }
    // params allocation:
    pthread_t    monitor;
    pthread_t *coder_ths = malloc(sizeof(pthread_t) * shared.params.num_coders);

    t_coder *coders = malloc(sizeof(t_coder ) * shared.params.num_coders);
    t_dongle   *dongles = malloc(sizeof(t_dongle) * shared.params.num_coders);
    shared.dongles = dongles;
    //function for the initialisation part:
    // dongle_init(&shared);
    // coders_init(coders, &shared);
    // shared_init(&shared, coders, dongles);
    params_initialisation(&shared, coders, dongles);
    // function for the threads creation
    pthread_create(&monitor, NULL, monitoring, (void *)&shared);
    for (int i = 0; i < shared.params.num_coders; i++)
    {
        pthread_create(coder_ths + i, NULL, routine, (void *)&coders[i]);
    }

    // functoin for threads join 
    for (int i = 0; i < shared.params.num_coders; i++)
    {
        pthread_join(coder_ths[i], NULL);
    }
    pthread_join(monitor, NULL);

    // function for the clean destroys and the frees
    for (int i = 0; i < shared.params.num_coders; i++)
    {
        pthread_mutex_destroy(&shared.coders[i].compile_count_mutex);
    }
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