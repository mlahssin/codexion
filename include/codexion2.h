#ifndef CODEXION2_H
#define CODEXION2_H
# include <stdio.h>


# include <string.h>
# include <stdlib.h>
# include <pthread.h>
# include <unistd.h>



typedef struct  s_params
{
    int num_coders;
    int time_to_burnout;
    int time_to_compile;
    int time_to_debug;
    int time_to_refactor;
    int num_compiles;
    int cooldown;
    int scheduler_type;
}t_params;

typedef struct  s_dongle
{
    int used;   //if it is free -1 if not the index of coder which is using it 
    int released_at;//when the dongle is ready to be used
    pthread_mutex_t dongle_mutex;    
}t_dongle;


typedef struct  s_sim
{
    t_params    params;
    t_dongle    *dongles;
}t_sim;

typedef struct  s_coder
{
    int id;
    t_dongle *left_dongle;
    t_dongle *right_dongle;
    int compile_count;
    int last_compile_start;
    t_sim   *sim;
}t_coder;







int parse(int ac, char **av, t_params *p);



#endif