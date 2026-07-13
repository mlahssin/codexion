# include "codexion2.h"

void    set_stop(t_shared  *shared)
{
    pthread_mutex_lock(&shared->stop_mutex);
    shared->stop = 1;
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

int now_ms()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return  now.tv_sec * 1000 + now.tv_usec / 1000;
}

int elapsed_time(int start)
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
        if ( elapsed_time(shared->start) >= start + time / 1000)
            break;
        usleep(1000);
    }
}

bool is_available(t_dongle   *dongle, int start)
{
    int value;
    value = 0;
    pthread_mutex_lock(&dongle->dongle_mutex);
    value = dongle->used;
    pthread_mutex_unlock(&dongle->dongle_mutex);
    return value == -1 && elapsed_time(start) >= dongle->released_at;
}



void    print_lock(int  id, int flag, t_shared *shared)
{
    pthread_mutex_lock(&shared->print_mutex);
    if (flag == 0)
        printf("%d %d has taken a dongle\n", elapsed_time(shared->start), id + 1);
    else if (flag == 1)
        printf("%d %d is compiling\n", elapsed_time(shared->start), id + 1);
    else if (flag == 2)
        printf("%d %d is debugging\n", elapsed_time(shared->start), id + 1);
    else if (flag == 3)
        printf("%d %d is refactoring\n", elapsed_time(shared->start), id + 1);
    else
    {
        printf("%d %d burned out\n", elapsed_time(shared->start), id + 1);

    }
    pthread_mutex_unlock(&shared->print_mutex);
}


bool    is_finish(t_coder   *coders, int num_coders)
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
        {
            return coders[i].id;
        }
        i++;
    }
    return -1;
}


void    use_dongle(t_dongle *dongle, int id)
{
    pthread_mutex_lock(&dongle->dongle_mutex);
    dongle->used = id;
    // pop(dongle);
    pthread_mutex_unlock(&dongle->dongle_mutex);
}

void    let_dongle(t_dongle *dongle, t_shared  *shared, int flag)
{
    pthread_mutex_lock(&dongle->dongle_mutex);
    dongle->used = -1;
    if (flag == 0)
        dongle->released_at = elapsed_time(shared->start) + shared->params.cooldown;
    pthread_mutex_unlock(&dongle->dongle_mutex);   
}

// void    try_use_dongle(t_dongle *dongle, t_coder    *coder)
// {
//     t_waiter    w;
//     w.coder_id = coder->id;
//     if (coder->shared->params.scheduler_type == 0)
//         w.prioroty = now_ms();
//     else
//         w.prioroty = coder->last_compile_start + coder->shared->params.time_to_burnout;
    
//     pthread_mutex_lock(&dongle->dongle_mutex);
//     push(dongle, w);
//     pthread_mutex_unlock(&dongle->dongle_mutex);
// }


void    *routine(void    *arg)
{                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  
    t_coder *coder = (t_coder   *)arg;

    while(get_stop(coder->shared) == 0)
    {
        // try_use_dongle(&coder->shared->dongles[coder->dongle_index_1], coder);
        //coder is the first elt of the heap and the dongle is available.
            if (is_available(&coder->shared->dongles[coder->dongle_index_1], coder->shared->start))
            {
                // printf("coder_id : %d\n", coder->id);
                use_dongle(&coder->shared->dongles[coder->dongle_index_1], coder->id);
                if(get_stop(coder->shared) == 1)
                {
                    return NULL;
                }
                use_dongle(&coder->shared->dongles[coder->dongle_index_1], coder->id);
                // try_use_dongle(&coder->shared->dongles[coder->dongle_index_1], coder);
            // printf("min = %d coder_id = %d\n", extract_min(&coder->shared->dongles[coder->dongle_index_2]), coder->id);
                if (is_available(&coder->shared->dongles[coder->dongle_index_2], coder->shared->start))
                {
                    if(get_stop(coder->shared) == 1)
                    {
                        return NULL;
                    }
                    
                    print_lock(coder->id, 0,coder->shared);
                    pthread_mutex_lock(&coder->shared->dongles[coder->dongle_index_2].dongle_mutex);
                    
                    if(get_stop(coder->shared) == 1)
                    {
                        return NULL;
                    }
                    print_lock(coder->id, 0,coder->shared);
                    coder->shared->dongles[coder->dongle_index_2].used = coder->id;
                    // pop(&coder->shared->dongles[coder->dongle_index_2]);
                    pthread_mutex_unlock(&coder->shared->dongles[coder->dongle_index_2].dongle_mutex);
                    coder->compile_count++;
                    if(get_stop(coder->shared) == 1)
                    {
                        return NULL;
                    }

                    print_lock(coder->id, 1,coder->shared);
                    coder->last_compile_start = elapsed_time(coder->shared->start);
                    smart_sleep(coder->shared->params.time_to_compile * 1000, coder->shared);
                    let_dongle(&coder->shared->dongles[coder->dongle_index_1], coder->shared, 0);
                    let_dongle(&coder->shared->dongles[coder->dongle_index_2], coder->shared, 0);
                    
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
                // else
                // {
                //     pthread_cond_wait(&coder->shared->dongles[coder->dongle_index_1].dongle_wait, &);
                // }
            else
            {
                if(get_stop(coder->shared) == 1)
                {
                    return NULL;
                }

                // pop(&coder->shared->dongles[coder->dongle_index_1]);
                let_dongle(&coder->shared->dongles[coder->dongle_index_1], coder->shared, 1);
            
            }
        }
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

        if(is_finish(shared->coders, shared->params.num_coders))
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
    pthread_mutex_init(&shared.print_mutex, NULL);
    pthread_mutex_init(&shared.stop_mutex, NULL);
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