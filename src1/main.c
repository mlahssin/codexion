# include "codexion2.h"

int is_available(t_dongle   dongle)
{
    return  dongle.used == -1;
}
void    dongle_init(t_dongle    *dongles, t_params  params)
{
    for (int i = 0; i < params.num_coders; i++)
    {
        dongles[i].used = -1;
        pthread_mutex_init(&dongles[i].dongle_mutex, NULL);
    }
}

void    coders_init(t_coder *coders, t_params    params, t_sim  *sim)
{
    for (int i = 0; i < params.num_coders; i++)
    {
        coders[i].id = i;
        coders[i].compile_count = 0;
        coders[i].right_dongle = NULL;
        coders[i].left_dongle = NULL;
        coders[i].last_compile_start = 0;
        coders[i].sim = sim;
    }

}
void    *simulation(void    *arg)
{
    t_coder *coder = (t_coder   *)arg;
    if (coder->id % 2 == 0)
    {
        pthread_mutex_lock(&coder->sim->dongles[coder->id].dongle_mutex);
        if (is_available(coder->sim->dongles[coder->id]))
        {
            coder->left_dongle = &coder->sim->dongles[coder->id];
            printf("coder %d takes dongle : %d\n", coder->id, coder->id);
            coder->sim->dongles[coder->id].used = coder->id;   
        }
        pthread_mutex_unlock(&coder->sim->dongles[coder->id].dongle_mutex);
    }
    else
    { 
        if (coder->id != coder->sim->params.num_coders - 1)
        {
            pthread_mutex_lock(&coder->sim->dongles[coder->id + 1].dongle_mutex);
            if (is_available(coder->sim->dongles[coder->id + 1]))
            {
                
                coder->right_dongle = &coder->sim->dongles[coder->id + 1];
                printf("coder %d takes dongle : %d\n", coder->id, coder->id + 1);
                coder->sim->dongles[coder->id + 1].used = coder->id;
            }
            pthread_mutex_unlock(&coder->sim->dongles[coder->id + 1].dongle_mutex);
        }
        else
        {
            pthread_mutex_lock(&coder->sim->dongles[0].dongle_mutex);
            if (is_available(coder->sim->dongles[0]))
            {
                
                coder->right_dongle = &coder->sim->dongles[0];
                printf("coder %d takes dongle : 0\n",coder->id);
                coder->sim->dongles[0].used = 0; 
               

            }
            pthread_mutex_unlock(&coder->sim->dongles[0].dongle_mutex);
        }
    }
    return NULL;
}


int main(int ac, char *av[])
{
    t_params    params;
    t_sim   sim;
    

    if (!parse(ac, av, &params))
    {
        write(2, "Error\n", 6);
        return (1);
    }
    pthread_t    coder_ths[params.num_coders];
    sim.params = params;
    t_coder coders[params.num_coders];
    

    t_dongle   dongles[params.num_coders];

    dongle_init(dongles, params);

    sim.dongles = dongles;
    coders_init(coders, params, &sim);
    
    for (int i = 0; i < params.num_coders; i++)
    {
        pthread_create(coder_ths + i, NULL, simulation, (void *)&coders[i]);
    }

    for (int i = 0; i < params.num_coders; i++)
    {
        pthread_join(coder_ths[i], NULL);
    }
    for (int i = 0; i < params.num_coders; i++)
    {
        pthread_mutex_destroy(&dongles[i].dongle_mutex);
    }

}