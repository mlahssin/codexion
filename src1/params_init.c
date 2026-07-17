# include "codexion.h"


int    dongles_init(t_shared   *shared)
{
    int i;
    int res;

    i = 0;
    while (i < shared->params.num_coders)
    {
        res = dongle_init(&shared->dongles[i]);
        if (res == -1)
        {
            clean_dongles(shared->dongles, 1, i - 1);
            return -1;
        }
        if (res == -2)
        {
            clean_dongles(shared->dongles, 0, i);
            return -1;
        }
        i++;
    }
    return shared->params.num_coders;
}


int    coders_init(t_coder *coders, t_shared  *shared)
{
    int i;

    i = 0;
    while ( i < shared->params.num_coders)
    {
        coders[i].id = i;
        coder_dongle_init(&coders[i], i, shared);
        coders[i].compile_count = 0;
        if (pthread_mutex_init(&coders[i].compile_count_mutex, NULL) != 0)
            return i;

        coders[i].last_compile_start = 0;
        coders[i].shared = shared;
        coders[i].num_dongles_held = 0;
        i++;
    }
    return shared->params.num_coders;
}


int    shared_init(t_shared  *shared, t_coder *coders, t_dongle  *dongles)
{
    shared->dongles = dongles;
    shared->coders = coders;
    shared->start = now_ms();
    shared->stop = 0;
    if (pthread_mutex_init(&shared->print_mutex, NULL) != 0)
        return -1;
    if (pthread_mutex_init(&shared->stop_mutex, NULL) != 0)
    {
        pthread_mutex_destroy(&shared->print_mutex);
        return -1;
    }
    return 0;
}

int    params_initialisation(t_shared  *shared, t_coder    *coders, t_dongle   *dongles)
{
    int n;
    int succ_dong;
    int succ_cods;
    int sh_check;

    n = shared->params.num_coders;
    succ_dong = dongles_init(shared);
    succ_cods = coders_init(coders, shared);
    sh_check = shared_init(shared, coders, dongles);
    if (succ_cods != n || succ_dong != n ||  sh_check != 0)
    {
        if (sh_check == 0)
            clean_shared(shared);
        if (succ_dong > 0)
            clean_dongles(dongles, 1, n - 1);
        clean_coders(coders, succ_cods);
        return -1;
    }
    return 0;
}