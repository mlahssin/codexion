# include "codexion2.h"

void    dongle_init(t_shared   *shared)
{
    
    for (int i = 0; i < shared->params.num_coders; i++)
    {
        shared->dongles[i].used = -1;
        shared->dongles[i].released_at = 0;
        pthread_mutex_init(&shared->dongles[i].dongle_mutex, NULL);
        pthread_cond_init(&shared->dongles[i].dongle_wait, NULL);
        shared->dongles[i].size = 0;
    }
}

void    coders_init(t_coder *coders, t_shared  *shared)
{
    
    for (int i = 0; i < shared->params.num_coders; i++)
    {
        coders[i].id = i;

        if(i % 2 == 0)
        {

            coders[i].dongle_index_1 = i;
            coders[i].dongle_index_2 = (i + 1) % (shared->params.num_coders);

        }
        else
        {
            usleep(1000);
            coders[i].dongle_index_1 = (i + 1) % (shared->params.num_coders);
            coders[i].dongle_index_2 = i;
        }

        coders[i].compile_count = 0;
        coders[i].last_compile_start = 0;
        coders[i].shared = shared;
    }
}

void    sim_init(t_shared  *shared, t_coder *coders, t_dongle  *dongles)
{
    shared->dongles = dongles;
    shared->coders = coders;
    shared->start = now_ms();
    shared->stop = 0;
    pthread_mutex_init(&shared->print_mutex, NULL);
    pthread_mutex_init(&shared->stop_mutex, NULL); 
}