// thread function
# include "codexion.h"

void *routine(void *arg)
{
    t_coder *coder = (t_coder *)arg;
    // long    start = coder->sim->start_time;


    while(1)
    {

        printf("%ld %d has taken a dongle\n", elapsed_ms(coder->start_time), coder->id);
        usleep(1000);
    }
}