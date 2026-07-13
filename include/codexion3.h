// # include <pthread.h>
// # include <stdio.h>
// # include <stdlib.h>
// # include <string.h>
// # include <unistd.h>
// # include <sys/time.h>
// 



// typedef struct s_params {
//     int     num_coders;        // number_of_coders and number of coders
//     int     time_to_burnout;   // time_to_burnout
//     int     time_to_compile;   // time_to_compile (hold 2 dongles)
//     int     time_to_debug;     // time_to_debug ater compling
//     int     time_to_refactor;  // time_to_refactor after debuging
//     int     num_compiles;      // number_of_compiles_required to stop
//     int     cooldown;          // dongle_cooldown (time so the dongle is ready to be reused)
//     int     scheduler_type;    // 0 for FIFO, 1 for EDF

// } t_params;

// // struct t_coder;
// // struct t_heap;
// // struct t_sim;


// typedef struct s_dongle
// {
//     int available;  // // 1 = free, 0 = taken
//     long long released_at; // time when it will be ready 
//     pthread_mutex_t mutex; // a dongle is a shared data so we should avoid the data race
//     pthread_cond_t  cond;
//     // t_heap  queue;//priority queue of coders waiting
// }t_dongle;

// typedef struct s_coder
// {
//     int id;
//     int compile_count;
//     int last_compile_start;
//     t_dongle *left;
//     t_dongle *right;
//     long    start_time;
//     // t_sim   *sim;
// }t_coder;








// typedef struct s_heap
// {
//     t_waiter    *data; //array of waiters stored as a tree
//     int         size; // how many waiters are currently in heap
//     int         capacity;// max waiter that can the heap hold
// }   t_heap;



// for the entire simulation
// struct s_sim
// {
//     t_params        params;       // all 8 arguments from user
//     t_coder         *coders;      // array of N coders
//     t_dongle        *dongles;
//     pthread_t *threads;// stores threads ids from pthread_create
//     int stopped; //0 keep running , 1 stop now
//     pthread_mutex_t stop_mutex; //stopped is shared betwen all threads
//     pthread_mutex_t log_mutex; //printf is not thread safe (2 threads printin on the same time)
//     long start_time;
// }t_sim;

// typedef struct s_waiter {
//     t_coder     *coder; // who is waiter --pointer to the coder
//     long long   priority;    // arrival timestamp (FIFO) or deadline (EDF)
//     int   tiebreak;    // coder->id for determinism lower id win
// } t_waiter;



// int parse(int ac, char **av, t_params *p);
// long  now_ms();
// long  elapsed_ms(long start);
// void *routine(void *arg);
// long parse_args(char *str);
// void fill_args(t_params *p, char **av);