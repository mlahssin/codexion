# include "codexion.h"

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


void    print_dongles_taken(t_coder    *coder)
{
    if (coder->num_dongles_held == 2)
    {
        print_lock(coder->id, 0, coder->shared);
        print_lock(coder->id, 0, coder->shared);
    }
}