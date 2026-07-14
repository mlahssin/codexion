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
        // printf("here!\n");
        if(get_stop(shared) == 1)
        {
            
            break;
        }
        if ( elapsed_time(shared->start) >= start + time / 1000)//check this
            break;
        usleep(1000);
    }
}

bool is_available(t_dongle   *dongle)
{
    int value;
    value = 0;
    // pthread_mutex_lock(&dongle->dongle_mutex);
    // printf("here!\n");
    value = dongle->used;
    // pthread_mutex_unlock(&dongle->dongle_mutex);
    return value == -1;
}


bool    cooldown_finished(t_dongle   *dongle, int start)
{
    // printf("here!\n");
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


bool    is_finish(t_coder   *coders, int num_coders)
{
    int i = 0;
    // while(get_stop(coders[i].shared) == 0)
    // {
        while(i < num_coders)
        {
            // printf("hello\n");
            if (coders[i].compile_count < coders[i].shared->params.num_compiles)
                return false;
            i++;
        }
    // }
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

void    let_dongle(t_dongle *dongle, t_shared  *shared)
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

void    try_use_dongle(t_dongle *dongle, t_coder    *coder)
{
    t_waiter    w;
    w.coder_id = coder->id;


    if (coder->shared->params.scheduler_type == 0)
    {
        w.prioroty = now_ms();
    }
    else
        w.prioroty = coder->last_compile_start + coder->shared->params.time_to_burnout;
    pthread_mutex_lock(&dongle->dongle_mutex);
    if (get_stop(coder->shared)!= 0)
    {
        pthread_mutex_unlock(&dongle->dongle_mutex);
        return;
    }
    push(dongle, w);

    // printf("dkhalt hooone\n");
    pthread_mutex_unlock(&dongle->dongle_mutex);
}
bool    only_cooldown(t_dongle *dongle, t_coder    *coder)
{
    // printf("hiiiiiiii3\n"); 
    return(extract_min(dongle) == coder->id && is_available(dongle) && !cooldown_finished(dongle, coder->shared->start));
}

bool    all_reasons(t_dongle *dongle, t_coder    *coder)
{
    // printf("hiiiiiiisdsi\n"); 
    // printf("heap first : %d and coder id: %d\n", extract_min(dongle), coder->id);
    // if (is_available(dongle))
    //     printf("true\n");
    // else
    //     printf("false\n");
    
    //     if (cooldown_finished(dongle, coder->shared->start))
    //     printf("true\n");
    // else
    //     printf("false\n");
    
    return(is_available(dongle)  && extract_min(dongle) == coder->id && cooldown_finished(dongle, coder->shared->start));
}

void    wait_time(struct timespec *ts, long long ms_from_now)
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

void    use_dongle(t_dongle *dongle, t_coder    *coder, int flag)
{
    pthread_mutex_lock(&dongle->dongle_mutex);
    struct timespec ts;
    long long time_left = dongle->released_at - elapsed_time(coder->shared->start);

    wait_time(&ts, time_left);

    while(get_stop(coder->shared) == 0)
    {
        // printf("hiiiiiiii\n"); 
        if(all_reasons(dongle, coder))
        {  
            //  printf("hiiiiiiii1\n");
            dongle->used = coder->id;
            pop(dongle);
            pthread_cond_broadcast(&dongle->dongle_wait);
            
            if (flag == 1)
            {
                print_lock(coder->id, 0,coder->shared);
                print_lock(coder->id, 0,coder->shared);
            }
            
            pthread_mutex_unlock(&dongle->dongle_mutex);
            
            return;
        }
        else if(only_cooldown(dongle, coder))
        {
            
            // while(only_cooldown(dongle, coder))
            // printf("hiiiiiiii\n"); 
            pthread_cond_timedwait(&dongle->dongle_wait, &dongle->dongle_mutex, &ts);
        }
        else
        {
            // printf("hiiiiiiii4\n"); 
            pthread_cond_wait(&dongle->dongle_wait, &dongle->dongle_mutex);
        }
    }
    // printf("hiiiiiiii2\n");
    // printf("hiiiiiiii\n");
    // if (get_stop(coder->shared) != 0)
    // {  
    //     pthread_mutex_unlock(&dongle->dongle_mutex);
    //     return;
    // }
 
    pthread_mutex_unlock(&dongle->dongle_mutex);
}


void    *routine(void    *arg)
{                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  
    t_coder *coder = (t_coder   *)arg;

    while(get_stop(coder->shared) == 0)
    {

        try_use_dongle(&coder->shared->dongles[coder->dongle_index_1], coder);
        use_dongle(&coder->shared->dongles[coder->dongle_index_1], coder, 0);
        try_use_dongle(&coder->shared->dongles[coder->dongle_index_2], coder);
        use_dongle(&coder->shared->dongles[coder->dongle_index_2], coder, 1);
        
        if(get_stop(coder->shared) == 1)
        {
            return NULL;
        }

        print_lock(coder->id, 1,coder->shared);
        coder->last_compile_start = elapsed_time(coder->shared->start);
        smart_sleep(coder->shared->params.time_to_compile * 1000, coder->shared);
        coder->compile_count++;
        let_dongle(&coder->shared->dongles[coder->dongle_index_1], coder->shared);
        let_dongle(&coder->shared->dongles[coder->dongle_index_2], coder->shared);
        
        if(get_stop(coder->shared) == 1)
        {
            return NULL;
        }

        print_lock(coder->id, 2,coder->shared);
        smart_sleep(coder->shared->params.time_to_debug * 1000, coder->shared);
        if(get_stop(coder->shared) == 1)
        {
            return NULL;
        }
        print_lock(coder->id, 3,coder->shared);
        smart_sleep(coder->shared->params.time_to_refactor * 1000, coder->shared);
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
            // printf("adsfa\n");
            set_stop(shared);
            // printf("dun burn \n");
            print_lock(burnout_coder, 4,shared);
            return NULL;
        }

        if(is_finish(shared->coders, shared->params.num_coders))
        {
            set_stop(shared);
            return NULL;
        }
        // printf("ana li dkhalt\n");
        smart_sleep(1000, shared); 
    }
    return NULL;
}

int main(int ac, char *av[])
{
    // t_params    params;
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
    // pthread_mutex_init(&shared.print_mutex, NULL);
    // pthread_mutex_init(&shared.stop_mutex, NULL);
    pthread_create(&monitor, NULL, monitoring, (void *)&shared);
    for (int i = 0; i < shared.params.num_coders; i++)
    {
        // printf("this is%d\n", i);
        pthread_create(coder_ths + i, NULL, routine, (void *)&coders[i]);
    }

    for (int i = 0; i < shared.params.num_coders; i++)
    {
        pthread_join(coder_ths[i], NULL);
    }
    pthread_join(monitor, NULL);

    for (int i = 0; i < shared.params.num_coders; i++)
    {
        pthread_mutex_destroy(&dongles[i].dongle_mutex);
    }

}