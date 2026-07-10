# include "codexion2.h"

void    set_stop(t_sim  *sim)
{
    pthread_mutex_lock(&sim->stop_mutex);
    sim->stop = 1;
    pthread_mutex_unlock(&sim->stop_mutex);
}

int    get_stop(t_sim  *sim)
{
    int value;
    pthread_mutex_lock(&sim->stop_mutex);
    value = sim->stop;
    pthread_mutex_unlock(&sim->stop_mutex);
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

void    smart_sleep(int time, t_sim *sim)
{
    int start = elapsed_time(sim->start);
    while(get_stop(sim) == 0)
    {
        if(get_stop(sim) == 1)
            break;
        if ( elapsed_time(sim->start) >= start + time / 1000)
            break;
        printf("slepp\n");
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

void    print_lock(int  id, int flag, t_sim *sim)
{
    pthread_mutex_lock(&sim->print_mutex);
    if (flag == 0)
        printf("%d %d has taken a dongle\n", elapsed_time(sim->start), id);
    else if (flag == 1)
        printf("%d %d is compiling\n", elapsed_time(sim->start), id);
    else if (flag == 2)
        printf("%d %d is debugging\n", elapsed_time(sim->start), id);
    else if (flag == 3)
        printf("%d %d is refactoring\n", elapsed_time(sim->start), id);
    else
    {
        printf("%d %d burned out\n", elapsed_time(sim->start), id);

    }
    pthread_mutex_unlock(&sim->print_mutex);
}


bool    is_finish(t_coder   *coders, int num_coders)
{
    int i = 0;
    
    while(i < num_coders)
    {
        if (coders[i].compile_count < coders[i].sim->params.num_compiles)
            return false;
        i++;
    }
    printf("here finished\n");
    return true;
}

int    is_burnout(t_coder  *coders, int num_coders)
{
    int i = 0;
    while (i < num_coders)
    {
        if(coders[i].last_compile_start + coders[i].sim->params.time_to_burnout <= elapsed_time(coders[i].sim->start))
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
    pthread_mutex_unlock(&dongle->dongle_mutex);
}

void    let_dongle(t_dongle *dongle, t_sim  *sim, int flag)
{
    pthread_mutex_lock(&dongle->dongle_mutex);
    dongle->used = -1;
    if (flag == 0)
        dongle->released_at = elapsed_time(sim->start) + sim->params.cooldown;
    pthread_mutex_unlock(&dongle->dongle_mutex);   
}

void    *simulation(void    *arg)
{
    printf("i am inside simulation\n");
    t_coder *coder = (t_coder   *)arg;
    // printf("hello!\n");
    while(get_stop(coder->sim) == 0)
    {
        if (is_available(&coder->sim->dongles[coder->dongle_index_1], coder->sim->start))
        {
            // printf("hello!\n");
            printf("index3 : %d\n", coder->sim->stop);
        
            if(get_stop(coder->sim) == 1)
            {
                pthread_mutex_unlock(&coder->sim->dongles[coder->dongle_index_1].dongle_mutex);
                pthread_mutex_unlock(&coder->sim->dongles[coder->dongle_index_2].dongle_mutex);
                break;
            }
            if (is_available(&coder->sim->dongles[coder->dongle_index_2], coder->sim->start))
            {
                // printf("hello!\n");
                if(get_stop(coder->sim) == 1)
                {
                    pthread_mutex_unlock(&coder->sim->dongles[coder->dongle_index_1].dongle_mutex);
                    pthread_mutex_unlock(&coder->sim->dongles[coder->dongle_index_2].dongle_mutex);

                    break;
                }
                use_dongle(&coder->sim->dongles[coder->dongle_index_1], coder->id);
                // if (is_available(&coder->sim->dongles[coder->dongle_index_2], coder->sim->start))
                // {
                if(get_stop(coder->sim) == 1)
                {
                    pthread_mutex_unlock(&coder->sim->dongles[coder->dongle_index_2].dongle_mutex);
                    pthread_mutex_unlock(&coder->sim->dongles[coder->dongle_index_1].dongle_mutex);
                    break;
                }
                print_lock(coder->id, 0,coder->sim);
                pthread_mutex_lock(&coder->sim->dongles[coder->dongle_index_2].dongle_mutex);
                
                if(get_stop(coder->sim) == 1)
                {
                    pthread_mutex_unlock(&coder->sim->dongles[coder->dongle_index_1].dongle_mutex);
                    pthread_mutex_unlock(&coder->sim->dongles[coder->dongle_index_2].dongle_mutex);
                    break;
                }
                print_lock(coder->id, 0,coder->sim);
                coder->sim->dongles[coder->dongle_index_2].used = coder->id;
                pthread_mutex_unlock(&coder->sim->dongles[coder->dongle_index_2].dongle_mutex);
                coder->compile_count++;
                if(get_stop(coder->sim) == 1)
                {
                    pthread_mutex_unlock(&coder->sim->dongles[coder->dongle_index_1].dongle_mutex);
                    pthread_mutex_unlock(&coder->sim->dongles[coder->dongle_index_2].dongle_mutex);
                    break;
                }
                print_lock(coder->id, 1,coder->sim);
                coder->last_compile_start = elapsed_time(coder->sim->start);
                usleep(coder->sim->params.time_to_compile * 1000);
                // smart_sleep(coder->sim->params.time_to_compile * 1000, coder->sim);
                let_dongle(&coder->sim->dongles[coder->dongle_index_1], coder->sim, 0);
                let_dongle(&coder->sim->dongles[coder->dongle_index_2], coder->sim, 0);
                if(get_stop(coder->sim) == 1)
                {
                    pthread_mutex_unlock(&coder->sim->dongles[coder->dongle_index_1].dongle_mutex);
                    pthread_mutex_unlock(&coder->sim->dongles[coder->dongle_index_2].dongle_mutex);
                    break;
                }
                print_lock(coder->id, 2,coder->sim);
                usleep(coder->sim->params.time_to_debug * 1000);
                // smart_sleep(coder->sim->params.time_to_debug * 1000, coder->sim);
                if(get_stop(coder->sim) == 1)
                {
                    pthread_mutex_unlock(&coder->sim->dongles[coder->dongle_index_1].dongle_mutex);
                    pthread_mutex_unlock(&coder->sim->dongles[coder->dongle_index_2].dongle_mutex);
                    break;
                }
                print_lock(coder->id, 3,coder->sim);
                // smart_sleep(coder->sim->params.time_to_refactor * 1000, coder->sim);
                usleep(coder->sim->params.time_to_refactor * 1000);
                    // else
                    //     pthread_mutex_unlock(&coder->sim->dongles[coder->dongle_index_2].dongle_mutex);
        }
        else
            {
                if(get_stop(coder->sim) == 1)
                {
                    pthread_mutex_unlock(&coder->sim->dongles[coder->dongle_index_1].dongle_mutex);
                    pthread_mutex_unlock(&coder->sim->dongles[coder->dongle_index_2].dongle_mutex);
                    break;
                }
                let_dongle(&coder->sim->dongles[coder->dongle_index_1], coder->sim, 0);
            }
        printf("index3 : %d\n", coder->sim->stop);
        }
    }
    return NULL;
}


void    *check_end(void *arg)
{
    printf("i am inside check end\n");
    t_sim   *sim = (t_sim *)arg;
    while(get_stop(sim) == 0)
    {
        printf("index2 : %d\n", sim->stop);
        printf("hello2!\n");
        int burnout_coder = is_burnout(sim->coders, sim->params.num_coders); 
        if(burnout_coder != -1)
        {
            set_stop(sim);
            print_lock(burnout_coder, 4,sim);
            return NULL;
        }

        if(is_finish(sim->coders, sim->params.num_coders))
        {

            set_stop(sim);
            return NULL;
        }
        printf("index2 : %d\n", sim->stop);
        // smart_sleep(1000, sim); 
        usleep(1000);
    }
    return NULL;
}

int main(int ac, char *av[])
{
    t_params    params;
    t_sim   sim;
    
    if (!parse(ac, av, &sim.params))
    {
        write(2, "Error\n", 6);
        return (1);
    }

    pthread_t    monitor;
    pthread_t *coder_ths = malloc(sizeof(pthread_t) * sim.params.num_coders);

    t_coder *coders = malloc(sizeof(t_coder ) * sim.params.num_coders);
    t_dongle   *dongles = malloc(sizeof(t_dongle) * sim.params.num_coders);
    sim.dongles = dongles;

    dongle_init(&sim);

    coders_init(coders, &sim);
    sim_init(&sim, coders, dongles);
    pthread_mutex_init(&sim.print_mutex, NULL);
    pthread_mutex_init(&sim.stop_mutex, NULL);
    pthread_create(&monitor, NULL, check_end, (void *)&sim);
    for (int i = 0; i < sim.params.num_coders; i++)
    {
        printf("this is%d\n", i);
        pthread_create(coder_ths + i, NULL, simulation, (void *)&coders[i]);
    }

    for (int i = 0; i < sim.params.num_coders; i++)
    {
        pthread_join(coder_ths[i], NULL);
    }
    pthread_join(monitor, NULL);

    for (int i = 0; i < sim.params.num_coders; i++)
    {
        pthread_mutex_destroy(&dongles[i].dongle_mutex);
    }

}