# include "codexion.h"

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
        if(get_stop(shared) == 1)
            break;
        if ( elapsed_time(shared->start) >= start + time)
            break;
        usleep(1000);
    }
}

void    build_timeout_ts(struct timespec *ts, long long ms_from_now)
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