#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#define NUM_RUNNERS 4

pthread_barrier_t start_line;

void *runner(void *arg)
{
    int id = *(int *)arg;

    // Simulate runners arriving at the starting line at different times
    sleep(id);   // runner 0 arrives instantly, runner 3 takes longest
    printf("Runner %d reached the starting line, waiting for others...\n", id);

    pthread_barrier_wait(&start_line);
    // Execution BLOCKS here until all NUM_RUNNERS threads call this.
    // Once the last one arrives, ALL threads unblock at the same time.

    printf("Runner %d: GO!! Running now.\n", id);

    return NULL;
}

int main()
{
    pthread_t threads[NUM_RUNNERS];
    int ids[NUM_RUNNERS];

    // Initialize barrier: exactly NUM_RUNNERS threads must call wait()
    // before any of them are released.
    pthread_barrier_init(&start_line, NULL, NUM_RUNNERS);

    for (int i = 0; i < NUM_RUNNERS; i++)
    {
        ids[i] = i;
        pthread_create(&threads[i], NULL, runner, &ids[i]);
    }

    for (int i = 0; i < NUM_RUNNERS; i++)
        pthread_join(threads[i], NULL);

    pthread_barrier_destroy(&start_line);
    return 0;
}