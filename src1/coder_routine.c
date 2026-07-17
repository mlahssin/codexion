# include "codexion.h"

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
    release_dongle(&coder->shared->dongles[coder->dongle_index_1], coder);
    release_dongle(&coder->shared->dongles[coder->dongle_index_2], coder);
}

void    *routine(void    *arg)
{                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  
    t_coder *coder;

    coder = (t_coder   *)arg;
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