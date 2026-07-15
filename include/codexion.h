#ifndef CODEXION_H
#define CODEXION_H

# include <limits.h>
# include <stdio.h>
# include <unistd.h>
# include <string.h>
# include <stdlib.h>
# include <pthread.h>
# include <unistd.h>
#include <stdbool.h>
# include <sys/time.h>
# include <time.h>

typedef struct s_coder t_coder;

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



typedef struct  s_waiter
{
    int coder_id;
    int prioroty;
}t_waiter;


// typedef struct s_heap 
// {
//     int capacity;
//     int size;
//     t_waiter *waiters;
// }t_heap;

typedef struct  s_dongle
{
    int used;
    int released_at;
    pthread_mutex_t dongle_mutex;
    t_waiter    waiters[2];  
    int size;
    pthread_cond_t  dongle_wait;
}t_dongle;

typedef struct  s_shared
{
    t_params    params;
    t_dongle    *dongles;
    t_coder *coders;
    pthread_mutex_t print_mutex;
    long long start;
    int stop;
    pthread_mutex_t stop_mutex;
    
}t_shared;


typedef struct  s_coder
{
    int id;
    int dongle_index_1;
    int dongle_index_2;
    int compile_count;
    int last_compile_start;
    int num_dongles_held;
    t_shared   *shared;

}t_coder;




int parse(int ac, char **av, t_params *p);

void    dongle_init(t_shared   *shared);
void    coders_init(t_coder *coders, t_shared  *shared);
void    shared_init(t_shared  *shared, t_coder *coders, t_dongle  *dongles);
long long     now_ms();
void    push(t_dongle   *dongle, t_waiter   w);
int pop(t_dongle  *dongle);
int extract_min(t_dongle *dongle);
void    params_initialisation(t_shared  *shared, t_coder    *coders, t_dongle   *dongles);


#endif