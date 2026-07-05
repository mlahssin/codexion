// #include <string.h>
// #include <stdio.h>
// #include <unistd.h>
# include "codexion.h"
// typedef struct s_params {
//     int     num_coders;        // number_of_coders
//     int     time_to_burnout;   // time_to_burnout
//     int     time_to_compile;   // time_to_compile
//     int     time_to_debug;     // time_to_debug
//     int     time_to_refactor;  // time_to_refactor
//     int     num_compiles;      // number_of_compiles_required
//     int     cooldown;          // dongle_cooldown
//     int     scheduler_type;    // 0 for FIFO, 1 for EDF
// } t_params;


// #include <limits.h>



int main(int ac, char **av)
{
    t_params    params;
    long start_time;
    if (!parse(ac, av, &params))
    {
        write(2, "Error\n", 6);
        return (1);
    }

    int i = 0;
    pthread_t coders[params.num_coders];
    start_time = now_ms();


    t_coder cd[params.num_coders];

    t_sim sim;
    sim.params = params;
    sim.coders = cd;
    sim.stopped = 0;
    sim.start_time = start_time;
    pthread_mutex_init(&sim.log_mutex, NULL);
    pthread_mutex_init(&sim.stop_mutex, NULL);

    int j = 0;
    while(j < params.num_coders)
    {
        cd[j].id = j;
        cd[j].last_compile_start = 0;
        cd[j].compile_count = 0;
        cd[j].left = NULL;
        cd[j].right = NULL;
        cd[j].start_time = start_time;
        j++;
    }

    j = 0;

    while (i < params.num_coders)
    {
        pthread_create(coders + i, NULL, routine, cd + j);
        i++;
        j++;
    }

    i = 0;

    while (i < params.num_coders)
    {
        pthread_join(coders[i], NULL);
        i++;
    }

    return (0);
}