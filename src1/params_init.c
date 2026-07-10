# include "codexion2.h"

void    dongle_init(t_sim   *sim)
{
    
    for (int i = 0; i < sim->params.num_coders; i++)
    {
        sim->dongles[i].used = -1;
        sim->dongles[i].released_at = 0;
        pthread_mutex_init(&sim->dongles[i].dongle_mutex, NULL);
    }
}

void    coders_init(t_coder *coders, t_sim  *sim)
{
    
    for (int i = 0; i < sim->params.num_coders; i++)
    {
        coders[i].id = i + 1;
        // printf("coder !: %d\n", coders[i].id);

        if(coders[i].id % 2 == 0)
        {

            coders[i].dongle_index_1 = coders[i].id;
            // printf("dongle 1!: %d\n", coders[i].dongle_index_1);
            // printf("equal : %d\n",(coders[i].id + 1) % (sim->params.num_coders));
            coders[i].dongle_index_2 = (coders[i].id + 1) % (sim->params.num_coders);
            // printf("dongle 2!: %d\n", coders[i].dongle_index_2);

        }
        else
        {
            usleep(1000);
            coders[i].dongle_index_1 = (coders[i].id + 1) % (sim->params.num_coders);
            coders[i].dongle_index_2 = coders[i].id;
        }

        coders[i].compile_count = 0;
        coders[i].round = 0;
        coders[i].last_compile_start = 0;
        coders[i].sim = sim;
    }
    }

void    sim_init(t_sim  *sim, t_coder *coders, t_dongle  *dongles)
{
    sim->dongles = dongles;
    sim->coders = coders;
    sim->start = now_ms();
    sim->stop = 0;
    pthread_mutex_init(&sim->print_mutex, NULL);
    pthread_mutex_init(&sim->stop_mutex, NULL); 
}