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
    long long prioroty;
}t_waiter;

typedef struct  s_dongle
{
    int used;
    long long released_at;
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
    pthread_mutex_t compile_count_mutex;
    int last_compile_start;
    int num_dongles_held;
    t_shared   *shared;
}t_coder;

int     parse(int ac, char **av, t_params *p);
int dongles_init(t_shared   *shared);
int    coders_init(t_coder *coders, t_shared  *shared);
int    shared_init(t_shared  *shared, t_coder *coders, t_dongle  *dongles);
long long       now_ms();
long long       elapsed_time(long long start);
void    smart_sleep(int time, t_shared *shared);
void    build_timeout_ts(struct timespec *ts, long long ms_from_now);
void    push(t_dongle   *dongle, t_waiter   w);
int     pop(t_dongle  *dongle);
int     extract_min(t_dongle *dongle);
int     dongle_init(t_dongle    *dongle);
void    clean_dongles(t_dongle   *dongles, int flag, int num);
void    coder_dongle_init(t_coder   *coder, int index, t_shared  *shared);
void    clean_coders(t_coder    *coders, int num);
void    clean_shared(t_shared  *shared);
int     params_initialisation(t_shared  *shared, t_coder    *coders, t_dongle   *dongles);
bool is_empty(t_dongle *dongle);
void    swap(t_waiter *a, t_waiter *b);
void    buble_up(t_dongle *dongle, int index);
int find_smallest(t_dongle   *dongle, int parent, int left_child, int right_child);
void    shift_down(t_dongle *dongle);
void    set_stop(t_shared  *shared);
int    get_stop(t_shared  *shared);
bool    all_coders_done_compiling(t_coder   *coders, int num_coders);
int    is_burnout(t_coder  *coders, int num_coders);
void    *monitoring(void *arg);
bool is_dongle_available(t_dongle   *dongle);
bool    cooldown_finished(t_dongle   *dongle, long long start);
bool    is_only_cooldown_left(t_dongle *dongle, t_coder    *coder);
bool    can_coder_take_dongle(t_dongle *dongle, t_coder    *coder);
void    release_dongle(t_dongle *dongle, t_coder  *coder);
void    push_coder_to_dongle_heap(t_dongle *dongle, t_coder    *coder);
void    print_dongles_taken(t_coder    *coder);
void    take_dongle(t_dongle *dongle, t_coder    *coder);
void    aquire_dongles(t_coder    *coder);
void    compile_phase(t_coder *coder);
void    debug_phase(t_coder *coder);
void    refactor_phase(t_coder  *coder);
void    release_both_dongles(t_coder *coder);
void    *routine(void    *arg);
void    print_lock(int  id, int flag, t_shared *shared);
void    print_dongles_taken(t_coder    *coder);
void    clean_all(t_shared  *shared, t_coder    *coders, pthread_t  *coders_threads);
void    free_allocation(t_shared  *shared, t_coder    *coders, pthread_t  *coders_threads);
int    params_allocation(t_coder   **coders, t_dongle   **dongles, pthread_t **coders_ths, t_shared   *shared);
int    create_coder_threads(pthread_t  *coders_th, t_shared    *shared, t_coder    *coders);
int join_coders_threads(pthread_t  *coders_th, t_shared    *shared, t_coder    *coders);
int create_monitor_thread(pthread_t *monitor, t_shared  *shared, t_coder    *coders, pthread_t  *coders_th);
int join_monitor_thread(pthread_t *monitor, t_shared  *shared, t_coder    *coders, pthread_t  *coders_th);
#endif