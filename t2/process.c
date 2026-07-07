#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

# include <sys/wait.h>
# include <sys/types.h>

// fork() creates a new child process by duplicating the calling process.
// After fork(), the parent and child have separate memory spaces.
// Changes made in the child do NOT affect the parent.
// Likewise, changes made in the parent do NOT affect the child.

// Why?

// When fork() is called:

// The operating system creates a child process that initially has a copy of the parent's memory.
// Both processes have their own independent address space.
// Modern operating systems use copy-on-write (COW),
//  meaning the memory pages are shared until one process modifies them.
//  At that point, the modified page is copied, so each process has its own version.
int main()
{
    int x = 1;
    int pid = fork();
    if (pid == -1)
        return 1;
    else if (pid != 0)
    {
        printf ("parent : process id %d\n", getpid());
        printf("value of x : %d\n", x);
        wait(NULL);
        // printf ("parent : process id %d\n", getpid());
        // printf("value of x : %d\n", x);
    }
    else
    {
        x++;
        printf ("child process id %d\n", getpid());
        printf("value of x : %d\n", x);
    }


}




















// int main() {
//     int shared_var = 10;
    
//     // Create the child process
//     pid_t pid = fork(); 

//     // 1. Check for failure
//     if (pid < 0) {
//         printf("Fork failed!\n");
//         return 1;
//     } 
    
//     // 2. Child process logic
//     else if (pid == 0) {
//         printf("I am the CHILD process! My PID is %d\n", getpid());
//         shared_var += 5; // Modifying the child's copy of the variable
//         printf("CHILD: shared_var is now %d\n", shared_var);
//     } 
    
//     // 3. Parent process logic
//     else {
//         printf("I am the PARENT process! My PID is %d. I created child %d\n", getpid(), pid);
        
//         // It is good practice for the parent to wait for the child to finish
//         // Otherwise, the child might become a "zombie" process.
//         wait(NULL); 
        
//         printf("PARENT: shared_var is still %d (unaffected by child)\n", shared_var);
//         printf("PARENT: Child has finished.\n");
//     }

//     return 0;
// }