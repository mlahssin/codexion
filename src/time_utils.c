// now_ms give the time in ms
// elapsed_ms return the elapsed time since the start in milliseconds
# include <sys/time.h>
# include <stdio.h>
# include <unistd.h>

// is the long can overflow || should i add the LL near to 1000 
// long now_ms(void)
// {
//     struct timeval now;
//     gettimeofday(&now, NULL);
//     return (now.tv_sec * 1000 + now.tv_usec / 1000);
// }


// long  elapsed_ms(long start)
// {
//     return (now_ms() - start);
// }


// int main()
// {
//     struct timeval start, end;

//     gettimeofday(&start, NULL);
//     sleep(2);
//     gettimeofday(&end, NULL);
//     printf("%ld\n", (start.tv_sec));
//     printf("%ld\n", (end.tv_sec));
//     printf("%ld\n", (end.tv_sec - start.tv_sec));

// }

// int main(void)
// {
//     long long start;

//     start = now_ms();
//     usleep(500000);    // sleep 500ms
//     printf("elapsed: %ld ms\n", elapsed_ms(start));
//     // should print: elapsed: 500 ms (give or take 2ms)
//     return (0);
// }