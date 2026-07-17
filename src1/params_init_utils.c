# include "codexion.h"

int dongle_init(t_dongle    *dongle)
{
    dongle->used = -1;
    dongle->released_at = 0;
    if (pthread_mutex_init(&dongle->dongle_mutex, NULL) != 0)
        return -1;
    if (pthread_cond_init(&dongle->dongle_wait, NULL) != 0)
        return -2;
    dongle->size = 0;
    return 0;
}

void    clean_dongles(t_dongle   *dongles, int flag, int num)
{
    int i;

    i = num;
        if (flag == 1)
        {
            while (i >= 0)
            {
                pthread_mutex_destroy(&dongles[i].dongle_mutex);
                pthread_cond_destroy(&dongles[i].dongle_wait);
                i--;
            }
        }
        else
        {
            while (i >=0)
            {
                pthread_mutex_destroy(&dongles[i].dongle_mutex);
                if (i < num)
                    pthread_cond_destroy(&dongles[i].dongle_wait);
                i--; 
            }

        }
}

void    coder_dongle_init(t_coder   *coder, int index, t_shared  *shared)
{
    if(index % 2 == 0)
    {
        coder->dongle_index_1 = index;
        coder->dongle_index_2 = (index + 1) % (shared->params.num_coders);
    }
    else
    {
        usleep(1000);
        coder->dongle_index_1 = (index + 1) % (shared->params.num_coders);
        coder->dongle_index_2 = index;
    }

}

void    clean_coders(t_coder    *coders, int num)
{
    int i;
    i = 0;
    while(i < num)
    {
        pthread_mutex_destroy(&coders[i].compile_count_mutex);
        i++;
    }
}

void    clean_shared(t_shared  *shared)
{
    pthread_mutex_destroy(&shared->stop_mutex);
    pthread_mutex_destroy(&shared->print_mutex);
}