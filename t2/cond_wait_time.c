#include <pthread.h>
#include <time.h>
#include <stdio.h>
# include <sys/time.h>


int now_ms()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return  now.tv_sec * 1000 + now.tv_usec / 1000;
}

int elapsed_time(int start)
{
    return now_ms() - start;
}


// int main() {
//     pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
//     pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

//     int time = now_ms();

//     int consumed = elapsed_time(time);
//     // clock_gettime(CLOCK_REALTIME, &ts);/
//     // ts.tv_sec += 3;   // build the deadline: now + 3 seconds

//     consumed
//     pthread_mutex_lock(&lock);
//     int rc = pthread_cond_timedwait(&cond, &lock, &ts);  // <-- &ts used here
//     pthread_mutex_unlock(&lock);

//     if (rc == ETIMEDOUT)
//         printf("No signal received — gave up after 3 seconds\n");
//     else
//         printf("Woke up because someone signaled\n");

//     return 0;
}